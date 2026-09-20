// Copyright (c) 2026 Håkon F. Fjellanger
// All rights reserved. See LICENSE file for details.

#ifndef MCON_SESSION_HELPERS
#define MCON_SESSION_HELPERS

#include "mcon/types.h"
#include "session_type.h"

/// @brief Reset a session, making it ready to be allocated to a new client.
/// @param mcon The mcon object that the session belongs to.
/// @param session The index of the session to reset.
void session_reset_after_close(const struct mcon *mcon, const mcon_session_idx session);

void session_prep_for_new_client(const struct mcon *mcon, const mcon_session_idx session,
                                 int client_socket_fd);

/// @brief Initialize a given session object to its initial state.
/// @param session The session object to initialize.
void session_init(struct mcon_session *session);

#endif // MCON_SESSION_HELPERS
