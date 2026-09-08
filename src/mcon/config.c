#include "mcon/config.h"
#include "mcon/limits.h"

const struct mcon_config mcon_config_default = {
    .io_uring_queue_size = 256,
    .session_count = 128,
    .edge_triggered_client_events = false,
    .io_queue_size = 512,
    .max_live_accept_sqes = 8,
};

bool mcon_config_validate(const struct mcon_config config) {
    return
        // Non-zero values
        config.io_uring_queue_size > 0 &&
        config.session_count > 0 &&
        config.io_queue_size > 0 &&
        config.max_live_accept_sqes > 0 &&

        // Checking imposed limits
        config.session_count <= mcon_max_session_count() &&
        config.max_live_accept_sqes <= config.io_uring_queue_size;
}
