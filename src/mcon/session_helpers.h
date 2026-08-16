#ifndef MCON_SESSION_HELPERS
#define MCON_SESSION_HELPERS

#include "mcon/types.h"

/// @brief Reset a session, making it ready to be allocated to a new client.
/// @param mcon The mcon object that the session belongs to.
/// @param session The index of the session to reset.
void session_reset(const struct mcon* mcon, const mcon_session_idx session);

#endif // MCON_SESSION_HELPERS
