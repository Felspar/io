#include <felspar/exceptions.hpp>
#include <felspar/io/warden.hpp>

#ifdef FELSPAR_POSIX_SOCKETS
#include <fcntl.h>
#endif


felspar::posix::fd felspar::io::warden::create_socket(
        int domain,
        int type,
        int protocol,
        felspar::source_location const &loc) {
    posix::fd s{::socket(domain, type, protocol)};
    if (not s) {
        throw felspar::stdexcept::system_error{
                get_error(), std::system_category(), "Error creating socket",
                loc};
    } else {
        do_prepare_socket(s.native_handle(), loc);
        return s;
    }
}


auto felspar::io::warden::create_pipe(felspar::source_location const &loc)
        -> pipe {
#ifdef FELSPAR_WINSOCK2
    auto server = create_socket(AF_INET, SOCK_STREAM, 0, loc);
    posix::bind(server, INADDR_LOOPBACK, 0);
    sockaddr name;
    int namelen = sizeof(sockaddr);
    if (getsockname(server.native_handle(), &name, &namelen) != 0) {
        throw felspar::stdexcept::system_error{
            io::get_error(), std::system_category(), "getsockname to determine port", loc};
    }
    posix::listen(server, 1, loc);

    auto write = create_socket(AF_INET, SOCK_STREAM, 0, loc);
    if (::connect(write.native_handle(), &name, namelen) != 0) {
        auto const error = io::get_error();
        if (error != WSAEWOULDBLOCK) {
            throw felspar::stdexcept::system_error{
                error, std::system_category(), "connecting write end part 1", loc};
        }
    } else {
        throw felspar::stdexcept::runtime_error{"Expected WSAEWOULDBLOCK when connecting write end", loc};
    }

    auto read = posix::fd{::accept(server.native_handle(), &name, &namelen)};
    if (not read) {
        throw felspar::stdexcept::runtime_error{"Didn't get a valid read socket from accept"};
    }

    return {std::move(read), std::move(write)};
#else
    int fds[2] = {};
    if (::pipe2(fds, O_NONBLOCK) == 0) {
        return {posix::fd{fds[0]}, posix::fd{fds[1]}};
    } else {
        throw felspar::stdexcept::system_error{
                io::get_error(), std::system_category(), "Creating pipe", loc};
    }
#endif
}
