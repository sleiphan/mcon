#include <gtest/gtest.h>

extern "C" {
#include <mcon/mcon.h>
#include <mcon/socket.h>
}

#include "helpers.h"

TEST(mcon, base_case) {
    struct mcon *mcon;
    if (mcon_create(&mcon, mcon_config_default))
        FAIL();

    int server_socket = mcon_create_listening_socket(8080, "0.0.0.0");
    if (server_socket < 0)
        FAIL();

    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd < 0)
        FAIL();

    if (mcon_start(mcon, server_socket, epoll_fd))
        FAIL();

    int client_fd = connect_to_server("0.0.0.0", 8080);
    const char payload[] = "Hello world!\n";
    char input_buffer[sizeof(payload)];
    memset(input_buffer, 0, sizeof(payload));

    if (write(client_fd, payload, sizeof(payload)) != sizeof(payload))
        FAIL();
    shutdown(client_fd, SHUT_WR);

    const unsigned int max_events = 32;
    struct epoll_event events[max_events];
    struct mcon_event mcon_events[max_events];

    bool client_done_writing = false;

    do {
        int events_ready = epoll_wait(epoll_fd, events, max_events, 5000);
        if (events_ready < 0) {
            if (errno == EINTR)
                continue;
            FAIL();
        }

        for (int epoll_event_idx = 0; epoll_event_idx < events_ready; epoll_event_idx++) {
            int num_mcon_events =
                mcon_process_event(mcon, events[epoll_event_idx], mcon_events, max_events);

            for (int i = 0; i < num_mcon_events; i++) {
                struct mcon_event evt = mcon_events[i];

                switch (evt.type) {
                case MCON_EVENT_NEW_CONNECTION:
                    break;
                case MCON_EVENT_CLOSE_COMPLETE:
                    break;

                case MCON_EVENT_READ_RDY:
                    mcon_session_read(mcon, evt.session, input_buffer, sizeof(input_buffer));
                    break;
                case MCON_EVENT_READ_COMPLETE:
                    EXPECT_STREQ(payload, input_buffer);
                    if (!evt.result)
                        mcon_session_close(mcon, evt.session);
                    break;
                case MCON_EVENT_REMOTE_HANGUP:
                    client_done_writing = true;
                    // mcon_session_drain(mcon, evt.session, sizeof(input_buffer));
                    break;
                case MCON_EVENT_DRAIN_COMPLETE:
                    if (evt.result == 0)
                        mcon_session_close(mcon, evt.session);
                    else if (evt.result > 0)
                        mcon_session_drain(mcon, evt.session, sizeof(input_buffer));
                    else
                        FAIL();
                    break;
                default:
                    FAIL();
                }
            }

            continue;
        }

        if (mcon_submit(mcon) < 0)
            FAIL();
    } while (mcon_active_session_count(mcon) > 0 || !client_done_writing);

    mcon_destroy(mcon);
}

TEST(config, default_config_is_valid) { EXPECT_TRUE(mcon_config_validate(mcon_config_default)); }
