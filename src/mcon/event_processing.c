// Copyright (c) 2026 Håkon F. Fjellanger
// All rights reserved. See LICENSE file for details.

#include <sys/epoll.h>

#include "event_processing.h"
#include "io_uring_actions.h"
#include "io_uring_entry.h"
#include "mcon/constants.h"

int mcon_process_server_socket_event(struct mcon *mcon, const uint32_t epoll_events,
                                     const struct mcon_epoll_entry epoll_data,
                                     struct mcon_event *events, unsigned int max_events) {
    if (epoll_events & EPOLLIN)
        mcon->state.accepts_requested = true;

    // TODO: Deal with the other events coming from the listening socket.

    return 0;
}
