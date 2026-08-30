#include <sys/epoll.h>

#include "event_processing.h"
#include "io_uring_entry.h"
#include "mcon/constants.h"
#include "mcon_helpers.h"

int mcon_process_server_socket_event(struct mcon* mcon, const uint32_t epoll_events, const struct mcon_epoll_entry epoll_data, struct mcon_event* events, unsigned int max_events) {
    if (epoll_events & EPOLLIN) {
        struct mcon_io_uring_entry io_entry = (struct mcon_io_uring_entry) {
            .operation = MCON_IO_SESSION_ACCEPT,
            .session = MCON_NO_SESSION,
        };
        struct io_uring_sqe accept_sqe;
        io_uring_prep_accept(&accept_sqe, mcon->server_socket_fd, NULL, NULL, 0);
        io_uring_sqe_set_data64(&accept_sqe, ((union mcon_io_uring_data) io_entry).data);
        io_queue_push(&mcon->io_queue, accept_sqe);

        mcon_submit_sqes(mcon);
    }

    // TODO: deal with all the other events

    return 0;
}
