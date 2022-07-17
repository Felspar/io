#include <felspar/io/posix.hpp>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/resource.h>
#include <sys/socket.h>


std::pair<std::size_t, std::size_t>
        felspar::posix::promise_to_never_use_select() {
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
}


void felspar::posix::listen(
        int fd, int backlog, felspar::source_location const &loc) {
    if (::listen(fd, backlog) == -1) {
        throw felspar::stdexcept::system_error{
                errno, std::system_category(), "listen error", loc};
    }
}

void felspar::posix::set_non_blocking(
        int sock, felspar::source_location const &loc) {
    if (int err =
                ::fcntl(sock, F_SETFL, ::fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
        err != 0) {
        throw felspar::stdexcept::system_error{
                errno, std::system_category(), "fcntl F_SETFL error", loc};
    }
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
