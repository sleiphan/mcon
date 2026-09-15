#ifndef MCON_IO_URING_ACTIONS
#define MCON_IO_URING_ACTIONS

#include <liburing.h>

#include "io_queue.h"
#include "io_uring_entry.h"
#include "mcon/constants.h"
#include "mcon_type.h"

int enqueue_accept(struct mcon *mcon);

#endif // MCON_IO_URING_ACTIONS
