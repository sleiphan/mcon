#include <gtest/gtest.h>

extern "C" {
#include <mcon/mcon.h>
#include <mcon/socket.h>
}

#include "helpers.h"

void mcon_instance_with_one_connected_client(struct mcon **mcon_dst, int *server_socket_dst,
                                             int *epoll_fd_dst, int *client_fd_dst) {
    struct mcon *mcon;
    if (mcon_create(&mcon, mcon_config_default))
        FAIL();

    int server_socket = mcon_create_listening_socket(8080, "0.0.0.0");
    EXPECT_GE(server_socket, 0);

    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    EXPECT_GE(epoll_fd, 0);

    if (mcon_start(mcon, server_socket, epoll_fd))
        FAIL();

    int client_fd = connect_to_server("0.0.0.0", 8080);
    EXPECT_GE(client_fd, 0);

    *mcon_dst = mcon;
    *server_socket_dst = server_socket;
    *epoll_fd_dst = epoll_fd;
    *client_fd_dst = client_fd;
}

TEST(session, disallow_multiple_live_reads) {
    struct mcon *mcon;
    int server_socket, epoll_fd, client_fd;

    mcon_instance_with_one_connected_client(&mcon, &server_socket, &epoll_fd, &client_fd);
    shutdown(client_fd, SHUT_WR);

    const int event_buf_size = 16;
    struct epoll_event epoll_events[event_buf_size];
    struct mcon_event mcon_events[event_buf_size];

    const unsigned int transport_buffer_size = 16;
    char transport_buffer[transport_buffer_size];

    bool operation_executed = false;

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
                case MCON_EVENT_READ_RDY:
                    if (operation_executed)
                        break;

                    EXPECT_GE(mcon_session_read(mcon, evt.session, transport_buffer,
                                                transport_buffer_size),
                              0);
                    EXPECT_LT(mcon_session_read(mcon, evt.session, transport_buffer,
                                                transport_buffer_size),
                              0);
                    EXPECT_NE(errno, 0);
                    operation_executed = true;
                    break;
                case MCON_EVENT_READ_COMPLETE:
                    if (!evt.result)
                        mcon_session_close(mcon, evt.session);
                    break;
                }
            }
        }

        EXPECT_GE(mcon_submit(mcon), 0);
    } while (mcon_active_session_count(mcon) || !operation_executed);

    mcon_destroy(mcon);

    close(epoll_fd), close(server_socket);
    close(client_fd);

    EXPECT_NE(operation_executed, false);
}

TEST(session, disallow_multiple_live_writes) {
    struct mcon *mcon;
    int server_socket, epoll_fd, client_fd;

    mcon_instance_with_one_connected_client(&mcon, &server_socket, &epoll_fd, &client_fd);
    shutdown(client_fd, SHUT_WR);

    const int event_buf_size = 16;
    struct epoll_event epoll_events[event_buf_size];
    struct mcon_event mcon_events[event_buf_size];

    const unsigned int transport_buffer_size = 16;
    char transport_buffer[transport_buffer_size];

    bool operation_executed = false;

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
                    if (operation_executed)
                        break;

                    EXPECT_GE(mcon_session_write(mcon, evt.session, transport_buffer,
                                                 transport_buffer_size),
                              0);
                    EXPECT_LT(mcon_session_write(mcon, evt.session, transport_buffer,
                                                 transport_buffer_size),
                              0);
                    EXPECT_NE(errno, 0);
                    operation_executed = true;
                    break;
                case MCON_EVENT_WRITE_COMPLETE:
                    mcon_session_close(mcon, evt.session);
                    break;
                }
            }
        }

        EXPECT_GE(mcon_submit(mcon), 0);
    } while (mcon_active_session_count(mcon) || !operation_executed);

    mcon_destroy(mcon);

    close(epoll_fd), close(server_socket);
    close(client_fd);

    EXPECT_NE(operation_executed, false);
}

TEST(session, disallow_multiple_live_closes) {
    struct mcon *mcon;
    int server_socket, epoll_fd, client_fd;

    mcon_instance_with_one_connected_client(&mcon, &server_socket, &epoll_fd, &client_fd);
    shutdown(client_fd, SHUT_WR);

    const int event_buf_size = 16;
    struct epoll_event epoll_events[event_buf_size];
    struct mcon_event mcon_events[event_buf_size];

    const unsigned int transport_buffer_size = 16;
    char transport_buffer[transport_buffer_size];

    bool operation_executed = false;

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
                    if (operation_executed)
                        break;

                    EXPECT_GE(mcon_session_close(mcon, evt.session), 0);
                    EXPECT_LT(mcon_session_close(mcon, evt.session), 0);
                    EXPECT_NE(errno, 0);
                    operation_executed = true;
                    break;
                }
            }
        }

        EXPECT_GE(mcon_submit(mcon), 0);
    } while (mcon_active_session_count(mcon) || !operation_executed);

    mcon_destroy(mcon);

    close(epoll_fd), close(server_socket);
    close(client_fd);

    EXPECT_NE(operation_executed, false);
}

TEST(session, disallow_close_while_reading) {
    struct mcon *mcon;
    int server_socket, epoll_fd, client_fd;

    mcon_instance_with_one_connected_client(&mcon, &server_socket, &epoll_fd, &client_fd);
    shutdown(client_fd, SHUT_WR);

    const int event_buf_size = 16;
    struct epoll_event epoll_events[event_buf_size];
    struct mcon_event mcon_events[event_buf_size];

    const unsigned int transport_buffer_size = 16;
    char transport_buffer[transport_buffer_size];

    bool operation_executed = false;

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
                    if (operation_executed)
                        break;

                    EXPECT_GE(mcon_session_read(mcon, evt.session, transport_buffer,
                                                transport_buffer_size),
                              0);

                    EXPECT_EQ(mcon_session_close(mcon, evt.session), -1);
                    EXPECT_NE(errno, 0);
                    operation_executed = true;
                    break;
                case MCON_EVENT_READ_COMPLETE:
                    mcon_session_close(mcon, evt.session);
                    break;
                }
            }
        }

        EXPECT_GE(mcon_submit(mcon), 0);
    } while (mcon_active_session_count(mcon) || !operation_executed);

    mcon_destroy(mcon);

    close(epoll_fd), close(server_socket);
    close(client_fd);

    EXPECT_NE(operation_executed, false);
}

TEST(session, disallow_close_while_writing) {
    struct mcon *mcon;
    int server_socket, epoll_fd, client_fd;

    mcon_instance_with_one_connected_client(&mcon, &server_socket, &epoll_fd, &client_fd);
    shutdown(client_fd, SHUT_WR);

    const int event_buf_size = 16;
    struct epoll_event epoll_events[event_buf_size];
    struct mcon_event mcon_events[event_buf_size];

    const unsigned int transport_buffer_size = 16;
    char transport_buffer[transport_buffer_size];

    bool operation_executed = false;

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
                    if (operation_executed)
                        break;

                    EXPECT_GE(mcon_session_write(mcon, evt.session, transport_buffer,
                                                 transport_buffer_size),
                              0);

                    EXPECT_EQ(mcon_session_close(mcon, evt.session), -1);
                    EXPECT_NE(errno, 0);
                    operation_executed = true;
                    break;
                case MCON_EVENT_WRITE_COMPLETE:
                    mcon_session_close(mcon, evt.session);
                    break;
                }
            }
        }

        EXPECT_GE(mcon_submit(mcon), 0);
    } while (mcon_active_session_count(mcon) || !operation_executed);

    mcon_destroy(mcon);

    close(epoll_fd), close(server_socket);
    close(client_fd);

    EXPECT_NE(operation_executed, false);
}
