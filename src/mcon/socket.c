#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "mcon/socket.h"

static int _prep_server_socket(
    const int family,
    const struct sockaddr* address,
    const socklen_t addrlen,
    const bool try_dual_stack
) {
    int socket_fd = socket(family, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("socket");
        return -1;
    }

    // Allow other sockets to listen on the same port.
    // Those sockets should listen on the same exact address
    // which is why we don't use SO_REUSEADDR.
    static const int ON = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &ON, sizeof(ON))) {
        perror("setsockopt");
        close(socket_fd);
        return -1;
    }

    // When binding an IPv6 wildcard socket, disabling IPV6_V6ONLY lets one
    // listener accept both native IPv6 and IPv4-mapped connections on
    // platforms that support dual-stack sockets.
    if (try_dual_stack) {
        if (setsockopt(socket_fd, IPPROTO_IPV6, IPV6_V6ONLY, &(int){0}, sizeof(int))) {
            perror("setsockopt(IPV6_V6ONLY)");
            close(socket_fd);
            return -1;
        }
    }

    // Bind the socket
    if (bind(socket_fd, address, addrlen)) {
        perror("bind");
        close(socket_fd);
        return -1;
    }

    if (listen(socket_fd, SOMAXCONN)) {
        perror("listen");
        close(socket_fd);
        return -1;
    }

    return socket_fd;
}

static bool _can_try_dual_stack(const struct addrinfo* address_info) {
    if (address_info->ai_family != AF_INET6) {
        return false;
    }

    const struct sockaddr_in6* address = (const struct sockaddr_in6*) address_info->ai_addr;
    return IN6_IS_ADDR_UNSPECIFIED(&address->sin6_addr);
}

int mcon_create_listening_socket(const uint16_t port, const char* bind_address) {
    char port_str[6];
    memset(port_str, 0, sizeof(port_str));
    snprintf(port_str, sizeof(port_str), "%d", port);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* address_info_result;
    int err = getaddrinfo(bind_address, port_str, &hints, &address_info_result);
    if (err != 0)
        return -1;

    int socket_fd = -1;

    // Prefer a dual-stack IPv6 wildcard listener first so one socket can
    // accept both IPv6 and IPv4-mapped connections when the platform allows it.
    for (struct addrinfo *rp = address_info_result; rp != NULL && socket_fd < 0; rp = rp->ai_next) {
        if (!_can_try_dual_stack(rp)) {
            continue;
        }

        socket_fd = _prep_server_socket(
            rp->ai_family,
            rp->ai_addr,
            rp->ai_addrlen,
            true
        );
    }

    // If no dual-stack candidate worked, fall back to the remaining addresses.
    for (struct addrinfo *rp = address_info_result; rp != NULL && socket_fd < 0; rp = rp->ai_next) {
        const bool try_dual_stack = _can_try_dual_stack(rp);
        if (try_dual_stack) {
            continue;
        }

        socket_fd = _prep_server_socket(
            rp->ai_family,
            rp->ai_addr,
            rp->ai_addrlen,
            false
        );
    }

    freeaddrinfo(address_info_result);

    return socket_fd;
}
