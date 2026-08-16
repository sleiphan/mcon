#include "mcon_helpers.h"
#include "mcon_type.h"

int mcon_submit_sqes(struct mcon* mcon) {
    int sqes_submitted = io_queue_pop_into_ring(&mcon->io_queue, &mcon->ring, mcon->configuration.io_uring_queue_size - mcon->state.sqe_in_flight);

    if (sqes_submitted > 0)
        mcon->state.sqe_in_flight += sqes_submitted;

    return sqes_submitted;
}
