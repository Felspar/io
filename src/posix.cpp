#include <felspar/io/posix.hpp>

#ifdef FELSPAR_POSIX_SOCKETS
#include <fcntl.h>
#include <sys/socket.h>
#endif


void felspar::posix::set_non_blocking(
        int sock, felspar::source_location const &loc) {
#if defined(FELSPAR_POSIX_SOCKETS)
    if (int const err =
                ::fcntl(sock, F_SETFL, ::fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
        err != 0) {
        throw felspar::stdexcept::system_error{
                errno, std::system_category(), "fcntl F_SETFL error", loc};
    }
#elif defined(FELSPAR_WINSOCK2)
    u_long mode = 1;
    if (int const err = ::ioctlsocket(sock, FIONBIO, &mode);
        err == SOCKET_ERROR) {
        throw felspar::stdexcept::system_error{
                WSAGetLastError(), std::system_category(),
                "ioctlsocket FIONBIO error", loc};
    }
#else
#error "No implementation for this platform"
#endif
}


void felspar::posix::set_reuse_port(
        int sock, felspar::source_location const &loc) {
    int optval = 1;
    if (::setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval))
        == -1) {
        throw felspar::stdexcept::system_error{
                errno, std::system_category(), "setsockopt SO_REUSEPORT failed",
                loc};
    }
}


void felspar::posix::bind_to_any_address(
        int const sock,
        std::uint16_t const port,
        felspar::source_location const &loc) {
    sockaddr_in in;
    in.sin_family = AF_INET;
    in.sin_port = htons(port);
    in.sin_addr.s_addr = htonl(INADDR_ANY);
    if (::bind(sock, reinterpret_cast<sockaddr const *>(&in), sizeof(in))
        != 0) {
        throw felspar::stdexcept::system_error{
                errno, std::system_category(), "Binding server socket", loc};
    }
}
