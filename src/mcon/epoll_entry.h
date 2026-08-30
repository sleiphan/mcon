#ifndef MCON_EPOLL_ENTRY
#define MCON_EPOLL_ENTRY

#define MCON_EPOLL_ENTRY_INSTANCE_SIZE 16
#define MCON_EPOLL_ENTRY_GENERATION_SIZE 8
#define MCON_EPOLL_ENTRY_SOURCE_SIZE 4
#define MCON_EPOLL_ENTRY_SESSION_SIZE 20
#define MCON_EPOLL_ENTRY_RESERVED_SIZE 16

#define MCON_EPOLL_ENTRY_INSTANCE_SHIFT (64-MCON_EPOLL_ENTRY_INSTANCE_SIZE)
#define MCON_EPOLL_ENTRY_GENERATION_SHIFT (MCON_EPOLL_ENTRY_INSTANCE_SHIFT-MCON_EPOLL_ENTRY_GENERATION_SIZE)
#define MCON_EPOLL_ENTRY_SOURCE_SHIFT (MCON_EPOLL_ENTRY_GENERATION_SHIFT-MCON_EPOLL_ENTRY_SOURCE_SIZE)
#define MCON_EPOLL_ENTRY_SESSION_SHIFT (MCON_EPOLL_ENTRY_SOURCE_SHIFT-MCON_EPOLL_ENTRY_SESSION_SIZE)
#define MCON_EPOLL_ENTRY_RESERVED_SHIFT (MCON_EPOLL_ENTRY_SESSION_SHIFT-MCON_EPOLL_ENTRY_RESERVED_SIZE)

#define MCON_EPOLL_ENTRY_INSTANCE_MASK   UINT64_C((1 << MCON_EPOLL_ENTRY_INSTANCE_SIZE)   - 1)
#define MCON_EPOLL_ENTRY_GENERATION_MASK UINT64_C((1 << MCON_EPOLL_ENTRY_GENERATION_SIZE) - 1)
#define MCON_EPOLL_ENTRY_SOURCE_MASK     UINT64_C((1 << MCON_EPOLL_ENTRY_SOURCE_SIZE)     - 1)
#define MCON_EPOLL_ENTRY_SESSION_MASK    UINT64_C((1 << MCON_EPOLL_ENTRY_SESSION_SIZE)    - 1)
#define MCON_EPOLL_ENTRY_RESERVED_MASK   UINT64_C((1 << MCON_EPOLL_ENTRY_RESERVED_SIZE)   - 1)



#include <sys/epoll.h>

#include "mcon/types.h"



enum mcon_epoll_source {
    MCON_EPOLL_SOURCE_UNKNOWN = 0,
    MCON_EPOLL_SOURCE_SERVER_SOCKET,
    MCON_EPOLL_SOURCE_TIMEOUT,
    MCON_EPOLL_SOURCE_IO_URING,
    MCON_EPOLL_SOURCE_SESSION,
};

struct mcon_epoll_entry {
    // A discriminator value to identify whether
    // an event belongs to a given mcon instance.
    uint16_t instance_id;

    uint8_t generation;

    enum mcon_epoll_source source;

    mcon_session_idx session;

    // Bytes reserved for future purposes
    uint16_t reserved;
};

static inline epoll_data_t mcon_encode_epoll_entry(const struct mcon_epoll_entry entry) {
    static const unsigned int instance_shift = MCON_EPOLL_ENTRY_INSTANCE_SHIFT;
    static const unsigned int generation_shift = MCON_EPOLL_ENTRY_GENERATION_SHIFT;
    static const unsigned int source_shift = MCON_EPOLL_ENTRY_SOURCE_SHIFT;
    static const unsigned int session_shift = MCON_EPOLL_ENTRY_SESSION_SHIFT;

    return (epoll_data_t) ((uint64_t) (
        ((uint64_t)entry.instance_id << MCON_EPOLL_ENTRY_INSTANCE_SHIFT) |
        ((uint64_t)entry.generation  << MCON_EPOLL_ENTRY_GENERATION_SHIFT) |
        ((uint64_t)entry.source      << MCON_EPOLL_ENTRY_SOURCE_SHIFT) |
        ((uint64_t)entry.session     << MCON_EPOLL_ENTRY_SESSION_SHIFT) |
        ((uint64_t)entry.reserved    << MCON_EPOLL_ENTRY_RESERVED_SHIFT)
    ));
}

static inline struct mcon_epoll_entry mcon_decode_epoll_entry(const epoll_data_t data) {
    return (struct mcon_epoll_entry) {
        .instance_id = (data.u64 >> MCON_EPOLL_ENTRY_INSTANCE_SHIFT)   & MCON_EPOLL_ENTRY_INSTANCE_MASK,
        .generation =  (data.u64 >> MCON_EPOLL_ENTRY_GENERATION_SHIFT) & MCON_EPOLL_ENTRY_GENERATION_MASK,
        .source =      (data.u64 >> MCON_EPOLL_ENTRY_SOURCE_SHIFT)     & MCON_EPOLL_ENTRY_SOURCE_MASK,
        .session =     (data.u64 >> MCON_EPOLL_ENTRY_SESSION_SHIFT)    & MCON_EPOLL_ENTRY_SESSION_MASK,
        .reserved =    (data.u64 >> MCON_EPOLL_ENTRY_RESERVED_SHIFT)   & MCON_EPOLL_ENTRY_RESERVED_MASK,
    };
}

#endif // MCON_EPOLL_ENTRY
