#include <stdint.h>
#include <sys/epoll.h>

static const uint32_t MCON_CLIENT_SOCKET_SUBSCRIBED_EVENTS =
    EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
