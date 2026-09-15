#ifndef MCON_EVENT
#define MCON_EVENT

#include "mcon/types.h"

enum mcon_event_type {
    MCON_EVENT_NONE = 0,
    MCON_EVENT_NEW_CONNECTION,
    MCON_EVENT_NEW_CONNECTION_FAILED,

    // Data has been received from a client, and it is
    // ready to be read.
    MCON_EVENT_READ_RDY,
    MCON_EVENT_READ_COMPLETE,
    MCON_EVENT_WRITE_COMPLETE,
    MCON_EVENT_DRAIN_COMPLETE,
    MCON_EVENT_CLOSE_COMPLETE,

    // The client associated with a session is done
    // sending data on this connection.
    MCON_EVENT_READ_SHUTDOWN,

    // The client closed the connection.
    MCON_EVENT_REMOTE_HANGUP,

    // A socket error happened on the socket connection
    // to the client.
    MCON_EVENT_CLIENT_ERROR,
};

struct mcon_event {
    int result;
    mcon_session_idx session;
    enum mcon_event_type type;
};

#endif // MCON_EVENT
