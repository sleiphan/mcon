#include "mcon_type.h"
#include "session_helpers.h"

void session_reset(const struct mcon* mcon, const mcon_session_idx session) {
    mcon->sessions[session].socket_fd = -1;
}
