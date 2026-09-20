// Copyright (c) 2026 Håkon F. Fjellanger
// All rights reserved. See LICENSE file for details.

#include <stdatomic.h>
#include <stdbool.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include "event_processing.h"
#include "io_uring_actions.h"
#include "mcon/constants.h"
#include "mcon/mcon.h"
#include "mcon/socket.h"
#include "mcon_type.h"
#include "session_helpers.h"

int mcon_create(struct mcon **dst, struct mcon_config config) {
    static _Atomic uint16_t instance_id_counter = 1;

    // Validate configuration
    if (!mcon_config_validate(config)) {
        errno = EINVAL;
        return -1;
    }

    // Setup io_uring instance
    struct io_uring ring;
    int io_uring_init_error = io_uring_queue_init(config.io_uring_queue_size, &ring, 0);
    if (io_uring_init_error) {
        errno = io_uring_init_error;
        goto error_io_uring;
    }

    // Create eventfd for listning to the io_uring instance
    int io_uring_eventfd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (io_uring_eventfd == -1)
        goto error_io_uring_event_fd;
    io_uring_register_eventfd(&ring, io_uring_eventfd);

    // Allocate and setup the io_queue
    struct io_queue io_queue;
    if (io_queue_create(&io_queue, config.io_queue_size))
        goto allocate_io_queue;

    struct mcon_session *sessions = malloc(config.session_count * sizeof(struct mcon_session));
    if (!sessions)
        goto allocate_sessions;

    struct idx_stack session_free_stack;
    if (idx_stack_create(&session_free_stack, config.session_count))
        goto allocate_session_free_stack;

    // Allocate memory for the mcon instance
    struct mcon *mcon = (struct mcon *)malloc(sizeof(struct mcon));
    if (!mcon)
        goto allocate_mcon_instance;

    for (mcon_session_idx i = 0; i < config.session_count; i++)
        session_init(sessions + i);

    for (mcon_session_idx i = 0; i < config.session_count; i++)
        idx_stack_push(&session_free_stack, (config.session_count - 1) - i);

    uint16_t instance_id = atomic_fetch_add_explicit(&instance_id_counter, 1, memory_order_relaxed);
    if (instance_id == 0)
        instance_id = atomic_fetch_add_explicit(&instance_id_counter, 1, memory_order_relaxed);

    // Populate mcon instance
    *mcon = (struct mcon){
        .instance_id = instance_id,
        .epoll_fd = -1,
        .server_socket_fd = -1,
        .io_uring_eventfd = io_uring_eventfd,
        .ring = ring,

        .configuration = config,
        .sessions = sessions,
        .session_free_stack = session_free_stack,

        .io_queue = io_queue,
        .state =
            (struct mcon_state){
                .sqe_in_flight = 0,
                .is_shutting_down = false,
                .accepts_live = 0,
                .accepts_requested = false,
            },
    };

    // Return mcon instance
    *dst = mcon;
    return 0;

// Error handling
allocate_mcon_instance:
    idx_stack_destroy(&session_free_stack);

allocate_session_free_stack:
    free(sessions);

allocate_sessions:
    io_queue_destroy(&io_queue);

allocate_io_queue:
    io_uring_unregister_eventfd(&ring);
    close(io_uring_eventfd);

error_io_uring_event_fd:
    io_uring_queue_exit(&ring);

error_io_uring:
    return -1;
}

void mcon_destroy(struct mcon *mcon) {
    idx_stack_destroy(&mcon->session_free_stack);
    free(mcon->sessions);
    io_queue_destroy(&mcon->io_queue);
    io_uring_unregister_eventfd(&mcon->ring);
    close(mcon->io_uring_eventfd);
    io_uring_queue_exit(&mcon->ring);
}

int mcon_start(struct mcon *mcon, int listening_socket_fd, int epoll_fd) {
    if (mcon_listening_socket_valid(listening_socket_fd)) {
        errno = EINVAL;
        return -1;
    }

    // Add the server socket to the interest list
    int err = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listening_socket_fd,
                        &(struct epoll_event){
                            .data = mcon_encode_epoll_entry((struct mcon_epoll_entry){
                                .instance_id = mcon->instance_id,
                                .generation = 0,
                                .source = MCON_EPOLL_SOURCE_SERVER_SOCKET,
                                .session = MCON_NO_SESSION,
                                .reserved = 0,
                            }),
                            .events = EPOLLIN | EPOLLERR | EPOLLET,
                        });

    if (err)
        return -1;

    // Add the io_uring eventfd to the interest list
    err = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, mcon->io_uring_eventfd,
                    &(struct epoll_event){
                        .data = mcon_encode_epoll_entry((struct mcon_epoll_entry){
                            .instance_id = mcon->instance_id,
                            .generation = 0,
                            .source = MCON_EPOLL_SOURCE_IO_URING,
                            .session = MCON_NO_SESSION,
                            .reserved = 0,
                        }),
                        .events = EPOLLIN | EPOLLERR,
                    });

    if (err) {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, listening_socket_fd, NULL);
        return -1;
    }

    // Store the file descriptors
    mcon->server_socket_fd = listening_socket_fd;
    mcon->epoll_fd = epoll_fd;

    return 0;
}

int mcon_shutdown(struct mcon *mcon) {
    if (mcon->state.is_shutting_down) {
        errno = EBUSY;
        return -1;
    }

    if (mcon->server_socket_fd == -1) {
        errno = EINVAL;
        return -1;
    }

    // Stop listening for events from the listening socket,
    // thus stopping any new connections from being accepted.
    int err = epoll_ctl(mcon->epoll_fd, EPOLL_CTL_DEL, mcon->server_socket_fd, NULL);
    if (err)
        return err; // Any other error than ENOENT and ENOMEM indicates an implementation bug.

    mcon->state.is_shutting_down = true;

    return 0;
}

int mcon_process_event(struct mcon *mcon, const struct epoll_event epoll_event,
                       struct mcon_event *events, unsigned int event_capacity) {
    if (!mcon_owns_event(mcon, epoll_event)) {
        errno = EINVAL;
        return -1;
    }

    const struct mcon_epoll_entry event_data = mcon_decode_epoll_entry(epoll_event.data);

    int err = -1;
    switch (event_data.source) {
    case MCON_EPOLL_SOURCE_SERVER_SOCKET:
        err = mcon_process_server_socket_event(mcon, epoll_event.events, event_data, events,
                                               event_capacity);
        break;
    case MCON_EPOLL_SOURCE_IO_URING:
        err = mcon_process_io_uring_event(mcon, events, event_capacity);
        break;
    case MCON_EPOLL_SOURCE_SESSION:
        err = mcon_process_session_event(mcon, epoll_event.events, event_data, events,
                                         event_capacity);
        break;
    }

    return err;
}

int mcon_submit(struct mcon *mcon) {
    // Enqueue accept() SQEs if needed
    if (mcon->state.accepts_requested)
        while (mcon->state.accepts_live < mcon->configuration.max_live_accept_sqes) {
            if (enqueue_accept(mcon))
                break;

            mcon->state.accepts_live++;
            mcon->state.accepts_requested = false;
        }

    // Pop and submit SQEs from the IO queue.
    const unsigned int sqe_limit =
        mcon->configuration.io_uring_queue_size - mcon->state.sqe_in_flight;
    const int sqes_popped = io_queue_pop_into_ring(&mcon->io_queue, &mcon->ring, sqe_limit);
    int sqes_submitted = 0;
    if (sqes_popped)
        sqes_submitted = io_uring_submit(&mcon->ring);

    if (sqes_submitted < 0) {
        errno = -sqes_submitted;
        return -1;
    } else {
        mcon->state.sqe_in_flight += sqes_submitted;
        return sqes_submitted;
    }
}

int mcon_get_server_socket(struct mcon *mcon) { return mcon->server_socket_fd; }

bool mcon_is_shutting_down(const struct mcon *mcon) { return mcon->state.is_shutting_down; }

bool mcon_owns_event(const struct mcon *mcon, struct epoll_event event) {
    const struct mcon_epoll_entry data = mcon_decode_epoll_entry(event.data);

    bool result = true;

    result &= data.instance_id == mcon->instance_id;
    result &= mcon_is_valid_source(data.source);

    return result;
}

mcon_session_idx mcon_active_session_count(const struct mcon *mcon) {
    return mcon->configuration.session_count - idx_stack_size(&mcon->session_free_stack);
}
