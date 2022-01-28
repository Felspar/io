#include <felspar/exceptions.hpp>
#include <felspar/poll/warden.hpp>


felspar::posix::fd felspar::poll::warden::create_socket(
        int domain, int type, int protocol, felspar::source_location loc) {
    posix::fd s{::socket(domain, type, protocol)};
    if (not s) {
        throw felspar::stdexcept::system_error{
                errno, std::generic_category(), "Creating server socket",
                std::move(loc)};
    } else {
        return s;
    }
}
