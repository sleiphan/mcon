#ifndef MCON_SESSION_TYPE
#define MCON_SESSION_TYPE

#include <stdint.h>

struct mcon_session {
    int socket_fd;
    uint8_t generation;
};

#endif // MCON_SESSION_TYPE
