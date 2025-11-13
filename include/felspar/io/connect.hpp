#pragma once


#include <felspar/io/addrinfo.hpp>
#include <felspar/io/warden.hpp>


namespace felspar::io {


    /// ## Attempt to connect to an address
    template<typename Socket>
    inline auto connect(
            warden &ward,
            Socket &&sock,
            sockaddr const *const addr,
            socklen_t const addrlen,
            std::optional<std::chrono::nanoseconds> const timeout = {},
            std::source_location const &loc = std::source_location::current()) {
        return ward.connect(
                std::forward<Socket>(sock), addr, addrlen, timeout, loc);
    }


    /// ## Return a new socket connected to an address
    warden::task<posix::fd> connect(
            warden &ward,
            char const *hostname,
            std::uint16_t port,
            std::optional<std::chrono::nanoseconds> timeout = {},
            std::source_location const &loc = std::source_location::current());
    /**
     * Goes through each host address associated with the `hostname` and tries
     * to connect to them in turn. Returns the first one that works. If all of
     * them fail it throws the exception associated with the last one tried.
     * Also throws if no hosts are returned.
     */


}
