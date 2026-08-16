#ifndef MCON_LIMITS
#define MCON_LIMITS

#include "mcon/types.h"

mcon_session_idx mcon_max_session_count(void);

mcon_uring_queue_size_t mcon_max_iteration_size(void);

#endif // MCON_LIMITS
