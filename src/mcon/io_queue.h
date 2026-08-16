#ifndef MCON_IO_QUEUE
#define MCON_IO_QUEUE

#include <liburing.h>

typedef struct io_uring_sqe io_queue_entry;

#define KQUEUE_NAME kqueue
#define KQUEUE_TYPE io_queue_entry
#define KQUEUE_NO_IMPLEMENTATION
#include <ctools/kqueue.h>
#undef KQUEUE_NO_IMPLEMENTATION



/// @brief A queue layer on top of io_uring.
/// This layer provides more space than the io_uring rings are allowed,
/// and submits the SQEs in prioritized order.
struct io_queue {
    struct kqueue queue;
};

int io_queue_create(struct io_queue* ioq, const uint32_t max_io_operations_in_flight);
void io_queue_destroy(struct io_queue* ioq);

int io_queue_push(struct io_queue* ioq, io_queue_entry entry);

/// @brief Submits _count_ SQEs into the given _ring_ in prioritized order. The 
/// prioritization is biased towards closing established connections. This puts
/// the close() operation at the top of the hierarchy, followed by drain(),
/// write(), read(), and then finally accept().
/// @param ioq 
/// @param ring The io_uring instance to submit the SQEs to.
/// @param count The maximum number of SQEs to submit to the _ring_.
/// @return On success, returns the number of SQEs submitted. On failure,
/// returns -1 and an error code in errno.
int io_queue_pop_into_ring(struct io_queue* ioq, struct io_uring* ring, const unsigned int count);

#endif // MCON_IO_QUEUE
