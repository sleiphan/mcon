#ifndef MCON_IO_URING_OPERATION
#define MCON_IO_URING_OPERATION

#include "mcon/types.h"

enum mcon_io_uring_operation {
    MCON_IO_UNKNOWN = 0,
    MCON_IO_SESSION_ACCEPT,
    MCON_IO_SESSION_READ,
    MCON_IO_SESSION_WRITE,
    MCON_IO_SESSION_DRAIN,
    MCON_IO_SESSION_CLOSE,
};

struct mcon_io_uring_entry {
    enum mcon_io_uring_operation operation;
    mcon_session_idx session;
};

_Static_assert(sizeof(struct mcon_io_uring_entry) == sizeof(io_uring_data_t),
               "mcon_io_uring_entry must fit perfectly into io_uring SQEs data field");

union mcon_io_uring_data {
    struct mcon_io_uring_entry entry;
    io_uring_data_t data;
};

#endif // MCON_IO_URING_OPERATION
