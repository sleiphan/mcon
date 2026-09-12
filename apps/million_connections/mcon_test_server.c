#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <mcon/mcon.h>
#include <mcon/socket.h>
#include <ctools/bitset.h>
#include <sys/socket.h>

int handle_command(
        const char* input_buffer,
        const unsigned int input_buffer_size,
        char* output_buffer,
        const unsigned int output_buffer_size,
        struct mcon* mcon
    ) {
    unsigned int search_target_index = 0;
    while (search_target_index < input_buffer_size && input_buffer[search_target_index++] != ' ');

    int is_status = strncmp(input_buffer, "status", search_target_index) == 0;

    if (is_status) {
        const int active_sessions = mcon_active_session_count(mcon);
        return snprintf(output_buffer, output_buffer_size, "active sessions: %d\n", active_sessions);
    }

    return 0;
}

int main() {
    const int max_concurrent_sessions = 1000000;
    const int epoll_event_batch_size = 256;
    const int mcon_event_batch_size = 512;

    struct mcon_config cfg = (struct mcon_config) {
        .edge_triggered_client_events = true,
        .io_queue_size = max_concurrent_sessions,
        .session_count = max_concurrent_sessions,
        .max_live_accept_sqes = 256,
        .io_uring_queue_size = 256,
    };

    struct mcon* mcon;
    if (mcon_create(&mcon, cfg)) {
        perror("mcon_create");
        return -1;
    }

    int listening_socket = mcon_create_listening_socket(8080, "0.0.0.0");
    if (mcon_listening_socket_valid(listening_socket)) {
        printf("Listening socket invalid");
        return -1;
    }

    int accept_rc = accept(listening_socket, NULL, NULL);
    int errno_value = errno;

    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd < 0) {
        perror("epoll_create1");
        return -1;
    }

    if (mcon_start(mcon, listening_socket, epoll_fd)) {
        perror("mcon_start");
        return -1;
    }

    char input_buffer[64];
    char output_buffer[512];

    struct bitset rdhup_fired;
    bitset_create(&rdhup_fired, max_concurrent_sessions);

    printf("Server is now running and listening for incoming connections:\n\
io_queue_size        = %d\n\
session_count        = %d\n\
max_live_accept_sqes = %d\n\
io_uring_queue_size  = %d\n",
cfg.io_queue_size, cfg.session_count, cfg.max_live_accept_sqes, cfg.io_uring_queue_size);

    while (true) {
        struct epoll_event epoll_events[epoll_event_batch_size];
        errno = 0;
        int num_epoll_events = epoll_wait(epoll_fd, epoll_events, epoll_event_batch_size, 5000);
        // if (errno == EINTR)
        //     continue;
        if (num_epoll_events < 0 && errno != EINTR) {
            perror("epoll_wait");
            return -1;
        }


        struct mcon_event mcon_events[mcon_event_batch_size];
        for (int epoll_event_idx = 0; epoll_event_idx < num_epoll_events; epoll_event_idx++) {
            int mcon_events_available = mcon_process_event(mcon, epoll_events[epoll_event_idx], mcon_events, mcon_event_batch_size);
            if (mcon_events_available < 0) {
                perror("mcon_process_event");
                return -1;
            }

            for (int i = 0; i < mcon_events_available; i++) {
                struct mcon_event evt = mcon_events[i];
                switch (evt.type) {
                    case MCON_EVENT_READ_RDY:
                        // mcon_session_read(mcon, evt.session, input_buffer, sizeof(input_buffer));
                        break;
                    case MCON_EVENT_READ_COMPLETE:
                        // int msg_size = handle_command(input_buffer, sizeof(input_buffer), output_buffer, sizeof(output_buffer), mcon); 
                        // if (msg_size > 0)
                        //     mcon_session_write(mcon, evt.session, output_buffer, msg_size);
                        break;

                    case MCON_EVENT_REMOTE_HANGUP:
                        if (bitset_get(&rdhup_fired, evt.session)) {
                            printf("multiple close()");
                            return -1;
                        }
                        bitset_assign(&rdhup_fired, evt.session, 1);
                        mcon_session_close(mcon, evt.session);
                        break;

                    case MCON_EVENT_CLOSE_COMPLETE:
                        if (evt.result != 0) {
                            perror("closing session");
                            return -1;
                        }
                        break;
                    case MCON_EVENT_READ_SHUTDOWN:
                        printf("EPOLLHUP\n");
                        break;
                    case MCON_EVENT_CLIENT_ERROR:
                        printf("MCON_EVENT_CLIENT_ERROR\n");
                        break;
                    case MCON_EVENT_NEW_CONNECTION:
                        bitset_assign(&rdhup_fired, evt.session, 0);
                        break;
                    default:
                        printf("unhandled event\n");
                        break;
                }
            }
        }

        if (mcon_submit(mcon) < 0) {
            perror("mcon_submit");
            return -1;
        }
    }

    return 0;
}