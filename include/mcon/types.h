#ifndef MCON_TYPES
#define MCON_TYPES

#include <stdint.h>

typedef uint32_t mcon_session_idx;

typedef uint16_t mcon_iteration_idx;

struct mcon;

struct mcon_config {
    mcon_session_idx session_count;
    mcon_iteration_idx iteration_size;
};

#endif // MCON_TYPES
