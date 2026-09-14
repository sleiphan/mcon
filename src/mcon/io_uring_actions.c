#include "io_uring_actions.h"

int enqueue_accept(struct mcon* mcon) {
    // Do not enqueue more accept() if all sessions are occupied.
    // Note that idx_stack_size returns the amount of free sessions.
    if (idx_stack_size(&mcon->session_free_stack) <= mcon->state.accepts_live) {
        errno = ENOBUFS;
        return -1;
    }

    // The data to attach to the SQE
    struct mcon_io_uring_entry io_entry = (struct mcon_io_uring_entry) {
        .operation = MCON_IO_SESSION_ACCEPT,
        .session = MCON_NO_SESSION,
    };

    // Create the accept() SQE
    struct io_uring_sqe accept_sqe;
    io_uring_initialize_sqe(&accept_sqe);
    io_uring_prep_accept(&accept_sqe, mcon->server_socket_fd, NULL, NULL, 0);
    accept_sqe.ioprio = IORING_ACCEPT_DONTWAIT; // TODO: local adaptation due to divergence between the container's and host's major kernel version. 
    io_uring_sqe_set_data64(&accept_sqe, ((union mcon_io_uring_data) io_entry).data);

    return io_queue_push(&mcon->io_queue, accept_sqe);
}
