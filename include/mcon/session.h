#ifndef MCON_SESSION
#define MCON_SESSION

#include <stdbool.h>

#include "mcon/types.h"

int mcon_session_read(struct mcon *mcon, mcon_session_idx session, void *buf, unsigned int count);
int mcon_session_write(struct mcon *mcon, mcon_session_idx session, const void *buf,
                       unsigned int count);
int mcon_session_drain(struct mcon *mcon, mcon_session_idx session, unsigned int count);
int mcon_session_close(struct mcon *mcon, mcon_session_idx session);
int mcon_session_detach(struct mcon *mcon, const mcon_session_idx session);
int mcon_session_get_socket(struct mcon *mcon, mcon_session_idx session);
bool mcon_session_is_active(struct mcon *mcon, mcon_session_idx session);

#endif // MCON_SESSION
