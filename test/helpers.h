#include <arpa/inet.h>
#include <sys/epoll.h>

int connect_to_server(const char *server_address, uint16_t port) {
    struct sockaddr_in server_address_sock;
    memset(&server_address_sock, 0, sizeof(server_address_sock));
    server_address_sock.sin_family = AF_INET;
    server_address_sock.sin_addr.s_addr = inet_addr(server_address);
    server_address_sock.sin_port = htons(port);

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("socket");
        return -1;
    }

    if (connect(client_fd, (sockaddr *)&server_address_sock, sizeof(server_address_sock)) != 0) {
        perror("socket");
        return -1;
    }

    return client_fd;
}
