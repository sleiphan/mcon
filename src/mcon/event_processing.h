#ifndef MCON_EVENT_PROCESSING
#define MCON_EVENT_PROCESSING



#include "mcon/event.h"
#include "mcon_type.h"

int mcon_process_io_uring_event(struct mcon* mcon, struct mcon_event* events, unsigned int max_events);

int mcon_process_server_socket_event(struct mcon* mcon, const uint32_t epoll_events, const struct mcon_epoll_entry epoll_data, struct mcon_event* events, unsigned int max_events);

int mcon_process_session_event(struct mcon* mcon, const uint32_t epoll_events, const struct mcon_epoll_entry epoll_data, struct mcon_event* events, unsigned int max_events);

#endif // MCON_EVENT_PROCESSING