#ifndef MCON_TYPES
#define MCON_TYPES

#include <stdint.h>

typedef unsigned long long io_uring_data_t;

typedef uint32_t mcon_session_idx;

typedef uint16_t mcon_uring_queue_size_t;

struct mcon;

struct mcon_config {
    mcon_session_idx session_count;

    /*
    The maximum amount of in-flight IO operations allowed
    */
    mcon_uring_queue_size_t io_uring_queue_size;

};

#endif // MCON_TYPES
