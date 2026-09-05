#include <sys/epoll.h>

#include "event_processing.h"
#include "io_uring_entry.h"
#include "mcon/constants.h"
#include "io_uring_actions.h"

int mcon_process_server_socket_event(struct mcon* mcon, const uint32_t epoll_events, const struct mcon_epoll_entry epoll_data, struct mcon_event* events, unsigned int max_events) {
    if (epoll_events & EPOLLIN)
        mcon->state.accepts_requested = true;

    // TODO: Deal with the other events coming from the listening socket.

    return 0;
}
