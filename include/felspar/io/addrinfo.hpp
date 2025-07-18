#pragma once


#include <felspar/coro/generator.hpp>
#include <felspar/io/socket_descriptor.hpp>

#include <string_view>


namespace felspar::io {


    /// ## Address information
    /**
     * Uses `getaddrinfo` to return the addresses associated with a given host.
     * Sets the port number so the `sockaddr` is ready for use with `connect`
     * calls.
     *
     * Note that the way `getaddrinfo` is used it will be blocking. This means
     * that using this may cause a processing dealay for other coroutines that
     * are also running.
     */
    felspar::coro::generator<std::pair<sockaddr *, socklen_t>>
            addrinfo(char const *, std::uint16_t);


}
