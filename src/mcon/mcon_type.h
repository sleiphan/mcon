#ifndef MCON_TYPE
#define MCON_TYPE


#include <liburing.h>

#define BITSET_NO_IMPLEMENTATION
#include <ctools/bitset.h>

#include "mcon/types.h"
#include "index_stack.h"
#include "epoll_entry.h"
#include "session_type.h"
#include "io_queue.h"

struct mcon_state {
    uint16_t sqe_in_flight;
    bool is_shutting_down;
};

struct mcon {
    uint16_t instance_id;

    int server_socket_fd;
    int epoll_fd;

    struct mcon_config configuration;

    struct mcon_session* sessions;
    mcon_session_idx active_session_count;
    struct idx_stack session_free_stack;

    struct io_uring ring;
    int io_uring_eventfd;

    struct io_queue io_queue;

    struct mcon_state state;
};

#endif // MCON_TYPE
