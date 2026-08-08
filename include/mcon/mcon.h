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
void mcon_destroy(struct mcon* mcon);

// Actions

int mcon_start(struct mcon* mcon);
int mcon_shutdown(struct mcon* mcon);
int mcon_process_event(struct mcon *mcon, const struct epoll_event *epoll_event, struct mcon_event *events, unsigned int event_capacity);

// Getters

int mcon_get_server_socket(struct mcon* mcon);
int mcon_is_shutting_down(const struct mcon* mcon);
int mcon_owns_event(const struct mcon* mcon, struct epoll_event event);
mcon_session_idx mcon_active_session_count(const struct mcon* mcon);

#endif // MCON_MCON
