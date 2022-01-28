#include <felspar/poll/posix.hpp>

#include <fcntl.h>
#include <sys/socket.h>


void felspar::posix::set_non_blocking(int sock, felspar::source_location loc) {
    if (int err =
                ::fcntl(sock, F_SETFL, ::fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
        err != 0) {
        throw felspar::stdexcept::system_error{
                errno, std::generic_category(), "fcntl F_SETFL error",
                std::move(loc)};
    }
}


void felspar::posix::set_reuse_port(int sock, felspar::source_location loc) {
    int optval = 1;
    if (::setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval))
        == -1) {
        throw felspar::stdexcept::system_error{
                errno, std::generic_category(),
                "setsockopt SO_REUSEPORT failed"};
    }
}
