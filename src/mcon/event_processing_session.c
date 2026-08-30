#include "event_processing.h"

int mcon_process_session_event(struct mcon* mcon, const uint32_t epoll_events, const struct mcon_epoll_entry epoll_data, struct mcon_event* events, unsigned int max_events) {
    // Check the validity of the session index value.
    if (epoll_data.session >= mcon->configuration.session_count) {
        errno = EINVAL;
        return -1;
    }

    // If this is a stale event, simply consume and skip it.
    if (epoll_data.generation == mcon->sessions[epoll_data.session].generation)
        return 0;

    // Processing...

    return 0;
}
