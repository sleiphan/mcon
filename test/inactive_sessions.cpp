#include <gtest/gtest.h>

extern "C" {
#include <mcon/mcon.h>
#include <mcon/socket.h>
}

#include "helpers.h"

TEST(session, sessions_start_out_inactive) {
    struct mcon *mcon;
    EXPECT_EQ(mcon_create(&mcon, mcon_config_default), 0);

    for (mcon_session_idx session = 0; session < mcon_config_default.session_count; session++)
        EXPECT_EQ(mcon_session_is_active(mcon, session), false);

    mcon_destroy(mcon);
}

TEST(session, operations_dont_execute_on_inactive_sessions) {
    struct mcon *mcon;
    EXPECT_EQ(mcon_create(&mcon, mcon_config_default), 0);

    const mcon_session_idx session_to_test = 0;
    const unsigned int buf_size = 8;
    char buf[buf_size];

    EXPECT_EQ(mcon_session_read(mcon, session_to_test, buf, buf_size), -1);
    EXPECT_EQ(mcon_session_write(mcon, session_to_test, buf, buf_size), -1);
    EXPECT_EQ(mcon_session_drain(mcon, session_to_test, buf_size), -1);
    EXPECT_EQ(mcon_session_close(mcon, session_to_test), -1);
    EXPECT_EQ(mcon_session_detach(mcon, session_to_test), -1);
    EXPECT_EQ(mcon_session_get_socket(mcon, session_to_test), -1);

    mcon_destroy(mcon);
}

TEST(session, active_indicator_reports_correct_value) {
    struct mcon_config cfg = mcon_config_default;
    cfg.session_count = 1;

    struct mcon *mcon;
    if (mcon_create(&mcon, cfg))
        FAIL();

    int server_socket = mcon_create_listening_socket(8080, "0.0.0.0");
    EXPECT_GE(server_socket, 0);

    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    EXPECT_GE(epoll_fd, 0);

    if (mcon_start(mcon, server_socket, epoll_fd))
        FAIL();

    int client_fd = connect_to_server("0.0.0.0", 8080);
    EXPECT_GE(client_fd, 0);
    shutdown(client_fd, SHUT_WR);

    const int event_buf_size = 16;
    struct epoll_event epoll_events[event_buf_size];
    struct mcon_event mcon_events[event_buf_size];

    const unsigned int transport_buffer_size = 16;
    char transport_buffer[transport_buffer_size];

    bool read_complete = false;
    bool write_complete = false;

    bool event_new_connection = false;
    bool event_read_rdy = false;
    bool event_read_complete = false;
    bool event_write_complete = false;
    bool event_close_complete = false;
    bool event_remote_hangup = false;

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
                    event_new_connection = true;
                    EXPECT_EQ(mcon_session_is_active(mcon, evt.session), true);
                    mcon_session_read(mcon, evt.session, transport_buffer, transport_buffer_size);
                    mcon_session_write(mcon, evt.session, transport_buffer, transport_buffer_size);
                    break;
                case MCON_EVENT_READ_RDY:
                    event_read_rdy = true;
                    EXPECT_EQ(mcon_session_is_active(mcon, evt.session), true);
                    break;
                case MCON_EVENT_READ_COMPLETE:
                    event_read_complete = true;
                    read_complete = true;
                    EXPECT_EQ(mcon_session_is_active(mcon, evt.session), true);

                    if (write_complete)
                        mcon_session_close(mcon, evt.session);
                    break;
                case MCON_EVENT_WRITE_COMPLETE:
                    event_write_complete = true;
                    write_complete = true;
                    EXPECT_EQ(mcon_session_is_active(mcon, evt.session), true);

                    if (read_complete)
                        mcon_session_close(mcon, evt.session);
                    break;
                case MCON_EVENT_CLOSE_COMPLETE:
                    event_close_complete = true;
                    EXPECT_EQ(mcon_session_is_active(mcon, evt.session), false);
                    break;
                case MCON_EVENT_REMOTE_HANGUP:
                    event_remote_hangup = true;
                    EXPECT_EQ(mcon_session_is_active(mcon, evt.session), true);
                    break;
                }
            }
        }

        EXPECT_GE(mcon_submit(mcon), 0);
    } while (mcon_active_session_count(mcon) || !read_complete || !write_complete);

    mcon_destroy(mcon);

    close(epoll_fd), close(server_socket);
    close(client_fd);

    EXPECT_EQ(event_new_connection, true);
    EXPECT_EQ(event_read_rdy, true);
    EXPECT_EQ(event_read_complete, true);
    EXPECT_EQ(event_write_complete, true);
    EXPECT_EQ(event_close_complete, true);
    EXPECT_EQ(event_remote_hangup, true);
}
