#pragma once


#if __has_include(<netinet/in.h>)

/// Standard POSIX system
#define FELSPAR_POSIX_SOCKETS
#include <netinet/in.h>

#elif __has_include(<winsock2.h>)

/// Windows POSIX like system
#define FELSPAR_WINSOCK2
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>

#endif


namespace felspar::io {


#if defined(FELSPAR_POSIX_SOCKETS)
    using socket_descriptor = int;
    constexpr socket_descriptor invalid_handle = -1;
#elif defined(FELSPAR_WINSOCK2)
    using socket_descriptor = SOCKET;
    constexpr socket_descriptor invalid_handle = INVALID_SOCKET;
#endif


}
