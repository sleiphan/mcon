#ifndef MCON_SOCKET
#define MCON_SOCKET

#include <stdint.h>

int mcon_create_listening_socket(const uint16_t port, const char* bind_address);

#endif // MCON_SOCKET
