// Copyright (c) 2026 Håkon F. Fjellanger
// All rights reserved. See LICENSE file for details.

#include "session_helpers.h"
#include "mcon_type.h"

void session_init(struct mcon_session *session) {
    *session = (struct mcon_session){
        .socket_fd = -1,
        .generation = 0,
        .state = MCON_SESSION_STATE_NONE,
    };
}

void session_reset_after_close(const struct mcon *mcon, const mcon_session_idx session) {
    mcon->sessions[session].socket_fd = -1;

    // Iterate session to next generation, causing any stale or leftover events to be discarded.
    uint8_t rollover = mcon->sessions[session].generation == 0xff;
    mcon->sessions[session].generation += !rollover;
    mcon->sessions[session].generation *= !rollover;
}

void session_prep_for_new_client(const struct mcon *mcon, const mcon_session_idx session,
                                 int client_socket_fd) {
    mcon->sessions[session].socket_fd = client_socket_fd;
    mcon->sessions[session].state = MCON_SESSION_STATE_NONE;
}
