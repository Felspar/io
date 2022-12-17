#include <felspar/exceptions.hpp>
#include <felspar/io/warden.hpp>


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


felspar::posix::pipe
        felspar::io::warden::create_pipe(felspar::source_location const &loc) {
#ifdef FELSPAR_WINSOCK2
    throw felspar::stdexcept::runtime_error{
            "Windows pipe creation is not implemented"};
#else
    std::array<int, 2> ends;
    if (::pipe(ends.data()) == -1) {
        throw felspar::stdexcept::system_error{
                errno, std::system_category(), "Error creating pipe", loc};
    } else {
        posix::pipe p{posix::fd{ends[0]}, posix::fd{ends[1]}};
        do_prepare_socket(p.read.native_handle(), loc);
        do_prepare_socket(p.write.native_handle(), loc);
        return p;
    }
#endif
}
