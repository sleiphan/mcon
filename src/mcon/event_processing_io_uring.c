
#include <liburing.h>
#include <sys/epoll.h>

#include "epoll_entry.h"
#include "io_uring_entry.h"
#include "mcon/event.h"
#include "mcon/constants.h"
#include "mcon_type.h"
#include "session_helpers.h"

int _process_accept(struct mcon* mcon, struct io_uring_cqe* cqe, struct mcon_event* events, unsigned int max_events) {
    // We need space to emit one event
    if (max_events < 1)
        return 0;

    // If the operation failed, notify the consumer.
    if (cqe->res < 0) {
        *events = (struct mcon_event) {
            .result = cqe->res,
            .session = MCON_NO_SESSION,
            .type = MCON_EVENT_NEW_CONNECTION,
        };

        return 1;
    }

    // Pop a free session
    mcon_session_idx new_session;
    if (idx_stack_pop(&mcon->session_free_stack, &new_session)) return -1;

    const int socket_fd = cqe->res;

    // Prepare the session
    session_prep_for_new_client(mcon, new_session, socket_fd);

    // Add socket to interest list
    int epoll_err = epoll_ctl(mcon->epoll_fd, EPOLL_CTL_ADD, socket_fd, &(struct epoll_event) {
        .data = mcon_encode_epoll_entry((struct mcon_epoll_entry) {
                .instance_id = mcon->instance_id,
                .generation = mcon->sessions[new_session].generation,
                .source = MCON_EPOLL_SOURCE_SESSION,
                .session = new_session,
                .reserved = 0,
            }),
        .events = EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR | EPOLLET,
    });

    // Cleanup if the call to epoll failed
    if (epoll_err) {
        mcon->sessions[new_session].socket_fd = -1;
        idx_stack_push(&mcon->session_free_stack, new_session);
        return -1;
    }

    // Increase the active sessions counter
    mcon->active_session_count++;

    // Register new event
    *events = (struct mcon_event) {
        .result = cqe->res,
        .session = new_session,
        .type = MCON_EVENT_NEW_CONNECTION,
    };

    return 1;
}

int _process_read(struct mcon* mcon, struct io_uring_cqe* cqe, const struct mcon_io_uring_entry io_entry, struct mcon_event* events, unsigned int max_events) {
    // We need space to emit one event
    if (max_events < 1)
        return 0;

    // Register new event
    *events = (struct mcon_event) {
        .result = cqe->res,
        .session = io_entry.session,
        .type = MCON_EVENT_READ_COMPLETE,
    };

    return 1;
}

int _process_drain(struct mcon* mcon, struct io_uring_cqe* cqe, const struct mcon_io_uring_entry io_entry, struct mcon_event* events, unsigned int max_events) {
    // We need space to emit one event
    if (max_events < 1)
        return 0;

    if (cqe->res > 0) {
        // TODO: limit the amount of bytes to drain
        // TODO: enqueue another read
        return 0;
    }

    *events = (struct mcon_event) {
        .result = cqe->res,
        .session = io_entry.session,
        .type = MCON_EVENT_DRAIN_COMPLETE,
    };

    return 1;
}

int _process_write(struct mcon* mcon, struct io_uring_cqe* cqe, const struct mcon_io_uring_entry io_entry, struct mcon_event* events, unsigned int max_events) {
    // We need space to emit one event
    if (max_events < 1)
        return 0;
    
    // Register new event
    *events = (struct mcon_event) {
        .result = cqe->res,
        .session = io_entry.session,
        .type = MCON_EVENT_WRITE_COMPLETE,
    };

    return 1;
}

int _process_close(struct mcon* mcon, struct io_uring_cqe* cqe, const struct mcon_io_uring_entry io_entry, struct mcon_event* events, unsigned int max_events) {
    if ((mcon->configuration.session_count - idx_stack_size(&mcon->session_free_stack)) < 1) {
        errno = ENOBUFS;
        return -1;
    }

    // We need space to emit one event
    if (max_events < 1)
        return 0;

    // Register new event
    *events = (struct mcon_event) {
        .result = cqe->res,
        .session = io_entry.session,
        .type = MCON_EVENT_WRITE_COMPLETE,
    };

    // Decrease the active sessions counter
    mcon->active_session_count--;

    // Make the session ready for a new connection
    session_reset_after_close(mcon, io_entry.session);

    // Return the session to the free stack
    if (idx_stack_push(&mcon->session_free_stack, io_entry.session))
        return -1;

    return 1;
}

int mcon_process_io_uring_event(struct mcon* mcon, struct mcon_event* events, unsigned int max_events) {
    unsigned int event_count = 0;
    int err = -1;

    for (struct io_uring_cqe* cqe; (event_count < max_events) & (io_uring_peek_cqe(&mcon->ring, &cqe) == 0);) {
        const struct mcon_io_uring_entry cqe_data = ((union mcon_io_uring_data) io_uring_cqe_get_data64(cqe)).entry;

        switch (cqe_data.operation) {
            case MCON_IO_SESSION_ACCEPT: err = _process_accept(mcon, cqe, &events[event_count], max_events - event_count); break;
            case MCON_IO_SESSION_READ:   err = _process_read(mcon, cqe, cqe_data, &events[event_count], max_events - event_count); break;
            case MCON_IO_SESSION_WRITE:  err = _process_write(mcon, cqe, cqe_data, &events[event_count], max_events - event_count); break;
            case MCON_IO_SESSION_DRAIN:  err = _process_drain(mcon, cqe, cqe_data, &events[event_count], max_events - event_count); break;
            case MCON_IO_SESSION_CLOSE:  err = _process_close(mcon, cqe, cqe_data, &events[event_count], max_events - event_count); break;
        }

        /*
        If the return value is less than zero, we can expect errno to be set to the correct error code.
        We should still return the amount of events produced by this call.
        And if the return value _is_ zero, there is not enough space in _events_ to process this CQE.
        */
        if (err < 1)
            break;

        // Consume the CQE, since we now have successfully processed it
        io_uring_cqe_seen(&mcon->ring, cqe);
        mcon->state.sqe_in_flight--;

        // Increase the event counter
        event_count += err;
    }

    return event_count;
}
