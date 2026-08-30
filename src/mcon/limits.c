#include "mcon/limits.h"

mcon_session_idx mcon_max_session_count(void) {
    return 1 << 20;
}

mcon_uring_queue_size_t mcon_max_iteration_size(void) {
    return 1 << 15;
}
