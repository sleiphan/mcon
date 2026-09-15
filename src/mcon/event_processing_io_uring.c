#include <liburing.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "constants.h"
#include "epoll_entry.h"
#include "io_uring_entry.h"
#include "mcon/constants.h"
#include "mcon/event.h"
#include "mcon/mcon.h"
#include "mcon_helpers.h"
#include "mcon_type.h"
#include "session_helpers.h"

int _process_accept(struct mcon *mcon, struct io_uring_cqe *cqe, struct mcon_event *events,
                    unsigned int max_events) {
    // If the SQE did not find any pending connection, simply skip and move on.
    if (cqe->res == -EAGAIN) {
        io_uring_cqe_seen(&mcon->ring, cqe); // Consume the CQE
        mcon->state.accepts_live--;
        return 0;
    }

    // We need space to emit one event
    if (max_events < 1)
        return 0;

    // Whatever happens from this point on, the CQE must be consumed.
    io_uring_cqe_seen(&mcon->ring, cqe);
    mcon->state.accepts_live--;

    // Whether the operation succeeds or errors from here, we have a signal
    // that the current accept() cycle should continue.
    mcon->state.accepts_requested = true;

    // Set new_session to MCON_NO_SESSION in case of errors.
    mcon_session_idx new_session = MCON_NO_SESSION;

    // If the operation failed, notify the consumer.
    if (cqe->res < 0)
        goto emit_event;

    // The new connection
    const int socket_fd = cqe->res;

    // Pop a free session
    if (idx_stack_pop(&mcon->session_free_stack, &new_session))
        return -1;

    // Add socket to interest list
    int epoll_err =
        epoll_ctl(mcon->epoll_fd, EPOLL_CTL_ADD, socket_fd,
                  &(struct epoll_event){
                      .data = mcon_encode_epoll_entry((struct mcon_epoll_entry){
                          .instance_id = mcon->instance_id,
                          .generation = mcon->sessions[new_session].generation,
                          .source = MCON_EPOLL_SOURCE_SESSION,
                          .session = new_session,
                          .reserved = 0,
                      }),
                      .events = MCON_CLIENT_SOCKET_SUBSCRIBED_EVENTS |
                                (mcon->configuration.edge_triggered_client_events ? EPOLLET : 0),
                  });

    // Return the session and disconnect the client if
    // we failed to add the socket to the interest list.
    if (epoll_err) {
        idx_stack_push(&mcon->session_free_stack, new_session);
        close(socket_fd);
        return -1;

        // Note that at this point, the CQE has been consumed, but the accept() cycle is still live.
        // So even if the call to epoll failed, the server can still try to accept new connections.
        // This is a useful reaction if e.g. the system is running out of memory.
    }

    // Prepare the session
    session_prep_for_new_client(mcon, new_session, socket_fd);

// Register new event
emit_event:
    *events = (struct mcon_event){
        .result = cqe->res,
        .session = new_session,
        .type = MCON_EVENT_NEW_CONNECTION,
    };

    return 1;
}

int _process_read(struct mcon *mcon, struct io_uring_cqe *cqe,
                  const struct mcon_io_uring_entry io_entry, struct mcon_event *events,
                  unsigned int max_events) {
    // We need space to emit one event
    if (max_events < 1)
        return 0;

    // Register new event
    *events = (struct mcon_event){
        .result = cqe->res,
        .session = io_entry.session,
        .type = MCON_EVENT_READ_COMPLETE,
    };

    // Consume the CQE
    io_uring_cqe_seen(&mcon->ring, cqe);

    // Clear the "reading" state from the session
    mcon->sessions[io_entry.session].state &= ~MCON_SESSION_STATE_READING;

    return 1;
}

int _process_write(struct mcon *mcon, struct io_uring_cqe *cqe,
                   const struct mcon_io_uring_entry io_entry, struct mcon_event *events,
                   unsigned int max_events) {
    // We need space to emit one event
    if (max_events < 1)
        return 0;

    // Register new event
    *events = (struct mcon_event){
        .result = cqe->res,
        .session = io_entry.session,
        .type = MCON_EVENT_WRITE_COMPLETE,
    };

    // Consume the CQE
    io_uring_cqe_seen(&mcon->ring, cqe);

    // Clear the "writing" state from the session
    mcon->sessions[io_entry.session].state &= ~MCON_SESSION_STATE_WRITING;

    return 1;
}

int _process_drain(struct mcon *mcon, struct io_uring_cqe *cqe,
                   const struct mcon_io_uring_entry io_entry, struct mcon_event *events,
                   unsigned int max_events) {
    // We need space to emit one event
    if (max_events < 1)
        return 0;

    // Register new event
    *events = (struct mcon_event){
        .result = cqe->res,
        .session = io_entry.session,
        .type = MCON_EVENT_DRAIN_COMPLETE,
    };

    // Consume the CQE
    io_uring_cqe_seen(&mcon->ring, cqe);

    // Clear the "reading" state from the session
    mcon->sessions[io_entry.session].state &= ~MCON_SESSION_STATE_READING;

    return 1;
}

int _process_close(struct mcon *mcon, struct io_uring_cqe *cqe,
                   const struct mcon_io_uring_entry io_entry, struct mcon_event *events,
                   unsigned int max_events) {
    // We need space to emit one event
    if (max_events < 1) {
        errno = ENOBUFS;
        return -1;
    }

    // Register new event
    *events = (struct mcon_event){
        .result = cqe->res,
        .session = io_entry.session,
        .type = MCON_EVENT_CLOSE_COMPLETE,
    };

    // Return the session to the free stack
    if (idx_stack_push(&mcon->session_free_stack, io_entry.session))
        return -1;

    // Make the session ready for a new connection
    session_reset_after_close(mcon, io_entry.session);

    // Consume the CQE
    io_uring_cqe_seen(&mcon->ring, cqe);

    // Clear the "closing" state from the session
    mcon->sessions[io_entry.session].state &= ~MCON_SESSION_STATE_CLOSING;

    if (mcon->state.is_shutting_down && mcon_active_session_count(mcon) == 0)
        mcon_complete_shutdown(mcon);

    return 1;
}

int mcon_process_io_uring_event(struct mcon *mcon, struct mcon_event *events,
                                unsigned int max_events) {
    unsigned int event_count = 0;
    unsigned int iteration_count = 0;

    for (struct io_uring_cqe *cqe;
         (event_count < max_events) & (io_uring_peek_cqe(&mcon->ring, &cqe) == 0);
         iteration_count++) {
        const struct mcon_io_uring_entry cqe_data =
            ((union mcon_io_uring_data)io_uring_cqe_get_data64(cqe)).entry;

        int err = -1;

        switch (cqe_data.operation) {
        case MCON_IO_SESSION_ACCEPT:
            err = _process_accept(mcon, cqe, &events[event_count], max_events - event_count);
            break;
        case MCON_IO_SESSION_READ:
            err =
                _process_read(mcon, cqe, cqe_data, &events[event_count], max_events - event_count);
            break;
        case MCON_IO_SESSION_WRITE:
            err =
                _process_write(mcon, cqe, cqe_data, &events[event_count], max_events - event_count);
            break;
        case MCON_IO_SESSION_DRAIN:
            err =
                _process_drain(mcon, cqe, cqe_data, &events[event_count], max_events - event_count);
            break;
        case MCON_IO_SESSION_CLOSE:
            err =
                _process_close(mcon, cqe, cqe_data, &events[event_count], max_events - event_count);
            break;
        default:
            break;
        }

        /*
        If the return value is less than zero, we can expect errno to be set to the correct error
        code. We should still return the amount of events produced by this call. And if the return
        value _is_ zero, there is not enough space in _events_ to process this CQE.
        */
        if (err < 0)
            break;

        // Decrease sqe counter if the cqe was consumed.
        struct io_uring_cqe *next_cqe;
        io_uring_peek_cqe(&mcon->ring, &next_cqe);
        mcon->state.sqe_in_flight -= cqe != next_cqe;

        // Increase the event counter
        event_count += err;
    }

    // Only clear the eventfd of the io_uring instance once we receive the io_uring-event
    // without processing _any_ CQEs.
    if (iteration_count == 0) {
        uint64_t output;
        read(mcon->io_uring_eventfd, &output, sizeof(output));

        // Trigger the eventfd if a CQE showed up while clearing the eventfd.
        if (io_uring_cq_ready(&mcon->ring) != 0) {
            uint64_t one = 1;
            write(mcon->io_uring_eventfd, &one, sizeof(one));
        }
    }

    return event_count;
}
