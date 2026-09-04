#include "mcon/config.h"

const struct mcon_config mcon_config_default = {
    .io_uring_queue_size = 256,
    .session_count = 128,
    .edge_triggered_client_events = false,
};
