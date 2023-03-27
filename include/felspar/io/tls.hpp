#pragma once


#include <felspar/io/warden.hpp>


namespace felspar::io {


    /// ## TLS secured TCP connection
    class tls final {
        struct impl;
        std::unique_ptr<impl> p;

        explicit tls(std::unique_ptr<impl>);

      public:
        tls();
        tls(tls const &) = delete;
        tls(tls &&);
        ~tls();

        tls &operator=(tls const &) = delete;
        tls &operator=(tls &&);

        /// ### Connect to a TLS secured server over TCP
        static warden::task<tls>
                connect(warden &,
                        sockaddr const *addr,
                        socklen_t addrlen,
                        std::optional<std::chrono::nanoseconds> timeout = {},
                        felspar::source_location const & =
                                felspar::source_location::current());
    };


}
