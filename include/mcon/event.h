#ifndef MCON_EVENT
#define MCON_EVENT

#include "mcon/types.h"

enum mcon_event_type {
    MCON_EVENT_NONE = 0,
    MCON_EVENT_NEW_CONNECTION,
    MCON_EVENT_NEW_CONNECTION_FAILED,
    MCON_EVENT_READ_RDY,
    MCON_EVENT_READ_COMPLETE,
    MCON_EVENT_WRITE_COMPLETE,
    MCON_EVENT_DRAIN_COMPLETE,
    MCON_EVENT_DRAIN_FAILURE,
    MCON_EVENT_CONNECTION_CLOSED,
    MCON_EVENT_READ_SHUTDOWN,
    MCON_EVENT_REMOTE_HANGUP,
};

struct mcon_event {
    int result;
    mcon_session_idx session;
    enum mcon_event_type type;
};

#endif // MCON_EVENT
