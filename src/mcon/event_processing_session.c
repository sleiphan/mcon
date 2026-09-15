#include <stdbit.h>

#include "constants.h"
#include "event_processing.h"

int mcon_process_session_event(struct mcon *mcon, const uint32_t epoll_events,
                               const struct mcon_epoll_entry epoll_data, struct mcon_event *events,
                               const unsigned int max_events) {
    // Check validity of events
    int valid_events = (epoll_events & ~MCON_CLIENT_SOCKET_SUBSCRIBED_EVENTS) == 0;

    // Check the validity of the session index value.
    int valid_index_value = epoll_data.session < mcon->configuration.session_count;

    if (!valid_events | !valid_index_value) {
        errno = EINVAL;
        return -1;
    }

    const unsigned num_events = stdc_count_ones(epoll_events);

    // Check that there is space enough in the events array
    if (max_events < num_events) {
        errno = ENOBUFS;
        return -1;
    }

    // If this is a stale event, simply consume and skip it.
    if (epoll_data.generation != mcon->sessions[epoll_data.session].generation)
        return 0;

    int events_produced = 0;

    if (epoll_events & EPOLLIN) {
        events[events_produced++] = (struct mcon_event){
            .result = 0,
            .session = epoll_data.session,
            .type = MCON_EVENT_READ_RDY,
        };
    }

    if (epoll_events & EPOLLHUP) {
        events[events_produced++] = (struct mcon_event){
            .result = 0,
            .session = epoll_data.session,
            .type = MCON_EVENT_READ_SHUTDOWN,
        };
    }

    if (epoll_events & EPOLLRDHUP) {
        if (!(mcon->sessions[epoll_data.session].state & MCON_SESSION_STATE_RDHUP))
            events[events_produced++] = (struct mcon_event){
                .result = 0,
                .session = epoll_data.session,
                .type = MCON_EVENT_REMOTE_HANGUP,
            };
        mcon->sessions[epoll_data.session].state |= MCON_SESSION_STATE_RDHUP;
    }

    int return_code = 0;

    if (epoll_events & EPOLLERR) {
        int err;
        socklen_t len = sizeof(err);
        return_code = getsockopt(mcon->sessions[epoll_data.session].socket_fd, SOL_SOCKET, SO_ERROR,
                                 &err, &len);

        events[events_produced++] = (struct mcon_event){
            .result = err,
            .session = epoll_data.session,
            .type = MCON_EVENT_CLIENT_ERROR,
        };
    }

    return !return_code ? events_produced : return_code;
}
