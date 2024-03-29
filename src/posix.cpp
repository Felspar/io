#include <felspar/io/posix.hpp>

#ifdef FELSPAR_POSIX_SOCKETS
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/resource.h>
#include <sys/socket.h>
#endif


std::pair<std::size_t, std::size_t>
        felspar::posix::promise_to_never_use_select() {
#ifdef FELSPAR_POSIX_SOCKETS
    ::rlimit limits;
    if (::getrlimit(RLIMIT_NOFILE, &limits) != 0) {
        throw felspar::stdexcept::system_error{
                errno, std::system_category(), "getrlimit RLIMIT_NOFILE error"};
    }
    if (limits.rlim_cur < limits.rlim_max) {
        std::size_t const curr = limits.rlim_cur;
        limits.rlim_cur = limits.rlim_max;
        if (::setrlimit(RLIMIT_NOFILE, &limits) != 0) {
            throw felspar::stdexcept::system_error{
                    errno, std::system_category(),
                    "setrlimit RLIMIT_NOFILE error"};
        }
        return {curr, limits.rlim_max};
    } else {
        return {limits.rlim_cur, limits.rlim_max};
    }
#else
    return {{}, {}};
#endif
}


void felspar::posix::listen(
        io::socket_descriptor fd,
        int backlog,
        felspar::source_location const &loc) {
    if (::listen(fd, backlog) == -1) {
        throw felspar::stdexcept::system_error{
                io::get_error(), std::system_category(), "listen error", loc};
    }
}

void felspar::posix::set_non_blocking(
        io::socket_descriptor sock, felspar::source_location const &loc) {
#if defined(FELSPAR_POSIX_SOCKETS)
    if (int const err =
                ::fcntl(sock, F_SETFL, ::fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
        err != 0) {
        throw felspar::stdexcept::system_error{
                io::get_error(), std::system_category(), "fcntl F_SETFL error",
                loc};
    }
#elif defined(FELSPAR_WINSOCK2)
    u_long mode = 1;
    if (int const err = ::ioctlsocket(sock, FIONBIO, &mode);
        err == SOCKET_ERROR) {
        throw felspar::stdexcept::system_error{
                io::get_error(), std::system_category(),
                "ioctlsocket FIONBIO error", loc};
    }
#else
#error "No implementation for this platform"
#endif
}


void felspar::posix::set_reuse_port(
        io::socket_descriptor const sock, felspar::source_location const &loc) {
    int optval = 1;
#if defined(FELSPAR_WINSOCK2)
    /// For Windows it looks like SO_REUSEADDR is the closest we can get
    constexpr auto reuse_flag = SO_REUSEADDR;
    char const *popt = reinterpret_cast<char const *>(&optval);
#else
    constexpr auto reuse_flag = SO_REUSEPORT;
    int *popt = &optval;
#endif
    if (::setsockopt(sock, SOL_SOCKET, reuse_flag, popt, sizeof(optval))
        == -1) {
        throw felspar::stdexcept::system_error{
                io::get_error(), std::system_category(),
                "setsockopt SO_REUSEPORT failed", loc};
    }
}


void felspar::posix::bind(
        io::socket_descriptor const sock,
        std::uint32_t const addr,
        std::uint16_t const port,
        felspar::source_location const &loc) {
    sockaddr_in in;
    in.sin_family = AF_INET;
    in.sin_port = htons(port);
    in.sin_addr.s_addr = htonl(addr);
    if (::bind(sock, reinterpret_cast<sockaddr const *>(&in), sizeof(in))
        != 0) {
        throw felspar::stdexcept::system_error{
                io::get_error(), std::system_category(),
                "Binding server socket", loc};
    }
}
