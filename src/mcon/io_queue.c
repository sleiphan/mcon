#include <assert.h>

#include "io_queue.h"
#include "io_uring_entry.h"

#define KQUEUE_NO_INTERFACE
#include <ctools/kqueue.h>
#undef KQUEUE_NO_INTERFACE



#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#define BLACKHOLE_IO_QUEUE_PRIORITY_GROUP_COUNT 5



int io_queue_create(struct io_queue* ioq, const uint32_t max_io_operations_in_flight) {
    struct kqueue q;
    if (kqueue_create(&q, max_io_operations_in_flight, BLACKHOLE_IO_QUEUE_PRIORITY_GROUP_COUNT))
        return -1;

    *ioq = (struct io_queue) {
        .queue = q
    };

    return 0;
}

void io_queue_destroy(struct io_queue* ioq) {
    kqueue_destroy(&ioq->queue);
}

int io_queue_push(struct io_queue* ioq, io_queue_entry entry) {
    const struct mcon_io_uring_entry sqe_data = ((union mcon_io_uring_data) entry.user_data).entry;
    unsigned int priority = 0;

    switch (sqe_data.operation) {
        case MCON_IO_SESSION_CLOSE:  priority = 0;
        case MCON_IO_SESSION_DRAIN:  priority = 1;
        case MCON_IO_SESSION_WRITE:  priority = 2;
        case MCON_IO_SESSION_READ:   priority = 3;
        case MCON_IO_SESSION_ACCEPT: priority = 4;
    }

    return kqueue_push(&ioq->queue, priority, entry);
}

int io_queue_pop_into_ring(struct io_queue* ioq, struct io_uring* ring, const unsigned int count) {
    const unsigned int ring_space = io_uring_sq_space_left(ring);
    const unsigned int queue_space = kqueue_size(&ioq->queue);
    const unsigned int space = MIN(ring_space, queue_space);
    const unsigned int limit = MIN(count, space);

    int enqueued = 0;

    for (int priority_group = 0; priority_group < BLACKHOLE_IO_QUEUE_PRIORITY_GROUP_COUNT; priority_group++) {
        struct io_uring_sqe sqe;

        while (enqueued < limit && kqueue_pop(&ioq->queue, priority_group, &sqe) == 0) {
            struct io_uring_sqe* sqe_dst = io_uring_get_sqe(ring);
            assert(sqe_dst != NULL);

            *sqe_dst = sqe;
            enqueued++;
        }
    }

    return enqueued;
}
