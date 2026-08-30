#include "mcon/config.h"

const struct mcon_config mcon_config_default = {
    .io_uring_queue_size = 32,
    .session_count = 128,
};
