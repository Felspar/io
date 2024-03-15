#pragma once


#include <felspar/io/warden.hpp>


namespace felspar::io {


    template<typename Socket>
    inline auto
            connect(warden &ward,
                    Socket &&sock,
                    sockaddr const *const addr,
                    socklen_t const addrlen,
                    std::optional<std::chrono::nanoseconds> const timeout = {},
                    felspar::source_location const &loc =
                            felspar::source_location::current()) {
        return ward.connect(
                std::forward<Socket>(sock), addr, addrlen, timeout, loc);
    }


}
