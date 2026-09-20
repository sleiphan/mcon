// Copyright (c) 2026 Håkon F. Fjellanger
// All rights reserved. See LICENSE file for details.

#include <gtest/gtest.h>

extern "C" {
#include <mcon/mcon.h>
#include <mcon/socket.h>
}

#include "helpers.h"

TEST(mcon, instance_works_normally_after_detach) {
    struct mcon *mcon;
    struct mcon_config cfg = mcon_config_default;
    cfg.session_count = 1;

    if (mcon_create(&mcon, cfg))
        FAIL();

    int server_socket = mcon_create_listening_socket(8080, "0.0.0.0");
    EXPECT_GE(server_socket, 0);

    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    EXPECT_GE(epoll_fd, 0);

    if (mcon_start(mcon, server_socket, epoll_fd))
        FAIL();

    int client_to_detach = connect_to_server("0.0.0.0", 8080);
    int client_to_process_normally;
    EXPECT_GE(client_to_detach, 0);
    shutdown(client_to_detach, SHUT_WR);

    const int event_buf_size = 16;
    struct epoll_event epoll_events[event_buf_size];
    struct mcon_event mcon_events[event_buf_size];

    const unsigned int transport_buffer_size = 16;
    char transport_buffer[transport_buffer_size];

    bool read_executed = false;
    bool detach_executed = false;

    do {
        int epoll_event_count = epoll_wait(epoll_fd, epoll_events, event_buf_size, 5000);
        EXPECT_GE(epoll_event_count, 0);

        for (int epoll_event_idx = 0; epoll_event_idx < epoll_event_count; epoll_event_idx++) {
            int mcon_event_count = mcon_process_event(mcon, epoll_events[epoll_event_idx],
                                                      mcon_events, event_buf_size);
            EXPECT_GE(mcon_event_count, 0);

            for (int mcon_event_idx = 0; mcon_event_idx < mcon_event_count; mcon_event_idx++) {
                struct mcon_event evt = mcon_events[mcon_event_idx];

                switch (evt.type) {
                case MCON_EVENT_NEW_CONNECTION:
                    if (!detach_executed) {
                        EXPECT_EQ(mcon_session_detach(mcon, evt.session), 0);
                        detach_executed = true;

                        client_to_process_normally = connect_to_server("0.0.0.0", 8080);
                        EXPECT_GE(client_to_process_normally, 0);
                        shutdown(client_to_process_normally, SHUT_WR);
                    }
                    break;
                case MCON_EVENT_READ_RDY:
                    mcon_session_read(mcon, evt.session, transport_buffer, transport_buffer_size);
                    break;

                case MCON_EVENT_READ_COMPLETE:
                    EXPECT_EQ(evt.result, 0);
                    if (evt.result == 0)
                        EXPECT_EQ(mcon_session_close(mcon, evt.session), 0);
                    read_executed = true;
                    break;
                }
            }
        }

        EXPECT_GE(mcon_submit(mcon), 0);
    } while (mcon_active_session_count(mcon) || !read_executed || !detach_executed);

    mcon_destroy(mcon);

    close(epoll_fd), close(server_socket);
    close(client_to_detach);
    close(client_to_process_normally);

    EXPECT_NE(detach_executed, false);
    EXPECT_NE(read_executed, false);
}
