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
    throw felspar::stdexcept::runtime_error{
            "Windows pipe creation is not implemented"};
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
