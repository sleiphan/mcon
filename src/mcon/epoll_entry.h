#ifndef MCON_EPOLL_ENTRY
#define MCON_EPOLL_ENTRY

#include "mcon/types.h"

enum mcon_epoll_source {
    MCON_EPOLL_SOURCE_UNKNOWN = 0,
    MCON_EPOLL_SOURCE_SERVER_SOCKET,
    MCON_EPOLL_SOURCE_TIMEOUT,
    MCON_EPOLL_SOURCE_IO_URING,
    MCON_EPOLL_SOURCE_SESSION,
};

struct mcon_epoll_entry {
    mcon_session_idx session;
    enum mcon_epoll_source source;
};

union mcon_epoll_data {
    struct mcon_epoll_entry entry;
    uint64_t data;
};

#endif // MCON_EPOLL_ENTRY
