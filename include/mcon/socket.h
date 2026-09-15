#ifndef MCON_SOCKET
#define MCON_SOCKET

#include <stdint.h>

int mcon_create_listening_socket(const uint16_t port, const char *bind_address);

enum mcon_socket_validation_error {
    MCON_SOCKET_VALIDATION_ERROR = -1,
    MCON_SOCKET_OK = 0,
    MCON_SOCKET_NOT_A_FILE_DESCRIPTOR,
    MCON_SOCKET_NOT_A_SOCKET,
    MCON_SOCKET_NOT_NONBLOCKING,
    MCON_SOCKET_NOT_STREAMING,
    MCON_SOCKET_NOT_LISTENING,
};

/**
 * @brief Checks whether the given listening socket is usable by mcon.
 * @param listening_socket The socket file descriptor to validate.
 * @return I the socket can be used by mcon, returns 0.
 * Otherwise, returns an mcon_socket_validation_error indicating why the socket cannot be used.
 */
enum mcon_socket_validation_error mcon_listening_socket_valid(const int listening_socket);

#endif // MCON_SOCKET
