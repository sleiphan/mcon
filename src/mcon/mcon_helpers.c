
#include "mcon_type.h"
#include "mcon_helpers.h"

void mcon_complete_shutdown(struct mcon* mcon) {
    // Reset file descriptors
    mcon->epoll_fd = -1;
    mcon->server_socket_fd = -1;

    // Reset shutdown tracker
    mcon->state.is_shutting_down = false;
}
