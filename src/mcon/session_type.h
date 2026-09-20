// Copyright (c) 2026 Håkon F. Fjellanger
// All rights reserved. See LICENSE file for details.

#ifndef MCON_SESSION_TYPE
#define MCON_SESSION_TYPE

#include <stdint.h>

enum mcon_session_state {
    MCON_SESSION_STATE_NONE = 0,
    MCON_SESSION_STATE_READING = 1 << 0,
    MCON_SESSION_STATE_WRITING = 1 << 1,
    MCON_SESSION_STATE_RDHUP = 1 << 2,
    MCON_SESSION_STATE_CLOSING = 1 << 3,
};

struct mcon_session {
    int socket_fd;
    uint8_t generation;
    uint8_t state;
};

#endif // MCON_SESSION_TYPE
