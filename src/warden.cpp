#include <felspar/exceptions.hpp>
#include <felspar/poll/warden.hpp>


int felspar::poll::warden::create_socket(int domain, int type, int protocol) {
    auto fd = ::socket(domain, type, protocol);
    if (fd < 0) {
        throw felspar::stdexcept::system_error{
                errno, std::generic_category(), "Creating server socket"};
    } else {
        return fd;
    }
}
