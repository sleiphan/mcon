// Copyright (c) 2026 Håkon F. Fjellanger
// All rights reserved. See LICENSE file for details.

#include "mcon/session.h"
#include "io_uring_entry.h"
#include "mcon/mcon.h"
#include "mcon_helpers.h"
#include "mcon_type.h"
#include "session_helpers.h"

#define VALIDATE_ARGS(incompatible_states)                                                         \
    do {                                                                                           \
        if (session >= mcon->configuration.session_count) {                                        \
            errno = EINVAL;                                                                        \
            return -1;                                                                             \
        }                                                                                          \
        if (!mcon_session_is_active(mcon, session) ||                                              \
            (mcon->sessions[session].state & (incompatible_states))) {                             \
            errno = EBUSY;                                                                         \
            return -1;                                                                             \
        }                                                                                          \
    } while (0)

static inline void _set_sqe_data(struct io_uring_sqe *sqe, mcon_session_idx session,
                                 enum mcon_io_uring_operation operation) {
    // Prepare data entry
    union mcon_io_uring_data sqe_data;
    sqe_data.entry = (struct mcon_io_uring_entry){
        .operation = operation,
        .session = session,
    };

    io_uring_sqe_set_data64(sqe, sqe_data.data);
}

int mcon_session_read(struct mcon *mcon, mcon_session_idx session, void *buf, unsigned int count) {
    VALIDATE_ARGS(MCON_SESSION_STATE_READING | MCON_SESSION_STATE_CLOSING);

    struct io_uring_sqe read_sqe;
    io_uring_initialize_sqe(&read_sqe);
    io_uring_prep_read(&read_sqe, mcon->sessions[session].socket_fd, buf, count, 0);
    _set_sqe_data(&read_sqe, session, MCON_IO_SESSION_READ);

    if (io_queue_push(&mcon->io_queue, read_sqe))
        return -1;

    mcon->sessions[session].state |= MCON_SESSION_STATE_READING;
    return 0;
}

int mcon_session_write(struct mcon *mcon, mcon_session_idx session, const void *buf,
                       unsigned int count) {
    VALIDATE_ARGS(MCON_SESSION_STATE_WRITING | MCON_SESSION_STATE_CLOSING);

    struct io_uring_sqe write_sqe;
    io_uring_initialize_sqe(&write_sqe);
    io_uring_prep_write(&write_sqe, mcon->sessions[session].socket_fd, buf, count, 0);
    _set_sqe_data(&write_sqe, session, MCON_IO_SESSION_WRITE);

    if (io_queue_push(&mcon->io_queue, write_sqe))
        return -1;

    mcon->sessions[session].state |= MCON_SESSION_STATE_WRITING;
    return 0;
}

int mcon_session_drain(struct mcon *mcon, mcon_session_idx session, unsigned int count) {
    VALIDATE_ARGS(MCON_SESSION_STATE_READING | MCON_SESSION_STATE_CLOSING);

    struct io_uring_sqe recv_sqe;
    io_uring_initialize_sqe(&recv_sqe);
    io_uring_prep_recv(&recv_sqe, mcon->sessions[session].socket_fd, &mcon->sessions[session],
                       count, MSG_TRUNC);
    _set_sqe_data(&recv_sqe, session, MCON_IO_SESSION_DRAIN);

    if (io_queue_push(&mcon->io_queue, recv_sqe))
        return -1;

    mcon->sessions[session].state |= MCON_SESSION_STATE_READING;
    return 0;
}

int mcon_session_close(struct mcon *mcon, mcon_session_idx session) {
    VALIDATE_ARGS(MCON_SESSION_STATE_READING | MCON_SESSION_STATE_WRITING |
                  MCON_SESSION_STATE_CLOSING);

    struct io_uring_sqe close_sqe;
    io_uring_initialize_sqe(&close_sqe);
    io_uring_prep_close(&close_sqe, mcon->sessions[session].socket_fd);
    _set_sqe_data(&close_sqe, session, MCON_IO_SESSION_CLOSE);

    if (io_queue_push(&mcon->io_queue, close_sqe))
        return -1;

    mcon->sessions[session].state |= MCON_SESSION_STATE_CLOSING;
    return 0;
}

int mcon_session_detach(struct mcon *mcon, const mcon_session_idx session) {
    VALIDATE_ARGS(MCON_SESSION_STATE_READING | MCON_SESSION_STATE_WRITING |
                  MCON_SESSION_STATE_CLOSING);

    // Return the session to the free stack
    if (idx_stack_push(&mcon->session_free_stack, session))
        return -1;

    const int epoll_error =
        epoll_ctl(mcon->epoll_fd, EPOLL_CTL_DEL, mcon->sessions[session].socket_fd, NULL);
    if (epoll_error) {
        if (errno != ENOENT) {
            // Pop the session again if the epoll operation failed to execute.
            mcon_session_idx session_tmp;
            idx_stack_pop(&mcon->session_free_stack, &session_tmp);
        }

        return -1;
    }

    // Make the session ready for a new connection
    session_reset_after_close(mcon, session);

    if (mcon->state.is_shutting_down && mcon_active_session_count(mcon) == 0)
        mcon_complete_shutdown(mcon);

    return 0;
}

int mcon_session_get_socket(struct mcon *mcon, mcon_session_idx session) {
    VALIDATE_ARGS(0);

    return mcon->sessions[session].socket_fd;
}

bool mcon_session_is_active(struct mcon *mcon, mcon_session_idx session) {
    if (session >= mcon->configuration.session_count) {
        errno = EINVAL;
        return -1;
    }

    return mcon->sessions[session].socket_fd != -1;
}
