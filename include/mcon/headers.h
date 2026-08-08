// config.h

#ifndef MCON_CONFIG
#define MCON_CONFIG

#include "mcon/types.h"

extern const struct mcon_config mcon_config_default;

#endif // MCON_CONFIG

// event.h

#ifndef MCON_EVENT
#define MCON_EVENT

#include "mcon/types.h"
#include <errno.h>

enum mcon_event_t {
    MCON_EVENT_NONE = 0,
    MCON_EVENT_NEW_CONNECTION,
    MCON_EVENT_READ_RDY,
    MCON_EVENT_READ_COMPLETE,
    MCON_EVENT_WRITE_COMPLETE,
    MCON_EVENT_DRAIN_COMPLETE,
    MCON_EVENT_CONNECTION_CLOSED,
    MCON_EVENT_SHUT_RD,
    MCON_EVENT_REMOTE_HANGUP,
};

struct mcon_event {
    mcon_session_idx session;
    enum mcon_event_t event;
    int return_value;
};

#endif // MCON_EVENT

// limits.h

#ifndef MCON_LIMITS
#define MCON_LIMITS

#include "mcon/types.h"

mcon_session_idx max_session_count();

mcon_iteration_idx max_iteration_size();

#endif // MCON_LIMITS

// mcon.h

#ifndef MCON_MCON
#define MCON_MCON


#include <sys/epoll.h>

// Gather the whole API into this file
#include "mcon/limits.h"
#include "mcon/config.h"
#include "mcon/session.h"
#include "mcon/event.h"


// Lifecycle

int mcon_create(struct mcon** dst, int listening_socket_fd, int epoll_fd, struct mcon_config config);
int mcon_create_default_listening_socket(const uint16_t port, const char* bind_address);
void mcon_destroy(struct mcon* mcon);

// Actions

int mcon_start(struct mcon* mcon);
int mcon_shutdown(struct mcon* mcon);
int mcon_process_event(struct mcon* mcon, struct epoll_event event, struct mcon_event* events_out, unsigned int events_out_capacity);

// Getters

int mcon_get_server_socket(struct mcon* mcon);
int mcon_is_shutting_down(const struct mcon* mcon);
int mcon_owns_event(const struct mcon* mcon, struct epoll_event event);
mcon_session_idx mcon_active_session_count(const struct mcon* mcon);

#endif // MCON_MCON

// session.h

#ifndef MCON_SESSION
#define MCON_SESSION

#include "mcon/types.h"

int mcon_session_read(struct mcon* mcon, mcon_session_idx session, void* buf, unsigned int count);
int mcon_session_write(struct mcon* mcon, mcon_session_idx session, const void* buf, unsigned int count);
int mcon_session_close(struct mcon* mcon, mcon_session_idx session);
int mcon_session_close_force(struct mcon* mcon, mcon_session_idx session);
int mcon_session_detach(struct mcon* mcon, mcon_session_idx session);
int mcon_session_get_socket(struct mcon* mcon, mcon_session_idx session);
void mcon_session_set_data(struct mcon* mcon, mcon_session_idx session, void* data);
void* mcon_session_get_data(struct mcon* mcon, mcon_session_idx session);

#endif // MCON_SESSION

// types.h

#ifndef MCON_TYPES
#define MCON_TYPES

#include <stdint.h>

typedef uint32_t mcon_session_idx;

typedef uint16_t mcon_iteration_idx;

struct mcon;

struct mcon_config {
    mcon_session_idx session_count;
    mcon_iteration_idx iteration_size;
};

#endif // MCON_TYPES
