#include <felspar/io/accept.hpp>
#include <felspar/io/addrinfo.hpp>
#include <felspar/io/pipe.hpp>
#include <felspar/io/read.hpp>
#include <felspar/io/warden.hpp>
#include <felspar/io/write.hpp>

#include <felspar/exceptions.hpp>

#include <sys/types.h>
#if defined(FELSPAR_POSIX_SOCKETS)
#include <sys/socket.h>
#include <netdb.h>
#endif


felspar::io::warden::stream<felspar::io::socket_descriptor> felspar::io::accept(
        warden &ward, socket_descriptor fd, felspar::source_location loc) {
    /**
     * Because this is a coroutine it must take the source location by copy not
     * by reference, as the referenced source location would go out of scope
     * before it can be used in this coroutine.
     */
    while (true) {
        auto s = co_await ward.accept(fd, {}, loc);
#if defined(FELSPAR_WINSOCK2)
        co_yield s;
#else
        if (s >= 0) {
            co_yield s;
        } else if (s != -EBADF) {
            throw felspar::stdexcept::system_error{
                    -s, std::system_category(), "accept", loc};
        } else {
            co_return;
        }
#endif
    }
}


std::size_t felspar::io::write_some(
        socket_descriptor sock,
        void const *const data,
        std::size_t const bytes) {
#ifdef FELSPAR_WINSOCK2
    if (auto const r =
                ::send(sock, reinterpret_cast<char const *>(data), bytes, {});
        r != SOCKET_ERROR) {
#else
    if (auto const r = ::write(sock, data, bytes); r >= 0) {
#endif
        return r;
    } else {
        throw felspar::stdexcept::system_error{
                get_error(), std::system_category(),
                "Writing to socket\n" + std::to_string(bytes)};
    }
}


felspar::coro::generator<std::pair<sockaddr *, socklen_t>> felspar::io::addrinfo(
        std::string_view const hostname, std::uint16_t const port) {
    struct getaddr {
        getaddr(std::string_view const hostname) {
            hints.ai_socktype = SOCK_STREAM;
            std::array<char, 256> hn{};
            std::copy(hostname.begin(), hostname.end(), hn.begin());
            if (getaddrinfo(hn.data(), nullptr, &hints, &addresses) != 0) {
                /// TODO Throw a proper error here
                throw felspar::stdexcept::runtime_error{
                        "getaddrinfo error looking up "
                        + std::string{hostname}};
            }
        }
        struct addrinfo hints = {};
        struct addrinfo *addresses = nullptr;
        ~getaddr() {
            if (addresses) { freeaddrinfo(addresses); }
        }
    };
    getaddr addrinfo{hostname};
    for (auto current = addrinfo.addresses; current != nullptr;
         current = current->ai_next) {
        felspar::posix::set_port(*current->ai_addr, port);
        co_yield std::pair{current->ai_addr, current->ai_addrlen};
    }
}
