// Copyright (c) 2026 Håkon F. Fjellanger
// All rights reserved. See LICENSE file for details.

#include "mcon_helpers.h"
#include "mcon_type.h"

void mcon_complete_shutdown(struct mcon *mcon) {
    // Reset file descriptors
    mcon->epoll_fd = -1;
    mcon->server_socket_fd = -1;

    // Reset shutdown tracker
    mcon->state.is_shutting_down = false;
}
