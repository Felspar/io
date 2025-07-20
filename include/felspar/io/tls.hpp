#pragma once


#include <felspar/io/addrinfo.hpp>
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
                        char const *snI_hostname,
                        sockaddr const *addr,
                        socklen_t addrlen,
                        std::optional<std::chrono::nanoseconds> timeout = {},
                        felspar::source_location const & =
                                felspar::source_location::current());
        static warden::task<tls> connect(
                warden &ward,
                char const *const hostname,
                std::uint16_t const port = 443,
                std::optional<std::chrono::nanoseconds> const timeout = {},
                felspar::source_location const &loc =
                        felspar::source_location::current());
        /**
         * Goes through each host address associated with the `hostname` and
         * tries to connect to them in turn. Returns the first one that works.
         * If all of them fail it throws the exception associated with the last
         * one tried. Also throws if no hosts are returned.
         */


        /// Read from the connection
        warden::task<std::size_t> read_some(
                warden &w,
                std::span<std::byte>,
                std::optional<std::chrono::nanoseconds> const timeout = {},
                felspar::source_location const & =
                        felspar::source_location::current());
        /// Write to the connection
        warden::task<std::size_t> write_some(
                warden &w,
                std::span<std::byte const>,
                std::optional<std::chrono::nanoseconds> const timeout = {},
                felspar::source_location const & =
                        felspar::source_location::current());
    };


    /// ## Free-standing APIs

    FELSPAR_CORO_WRAPPER inline auto read_some(
            warden &w,
            tls &cnx,
            std::span<std::byte> const s,
            std::optional<std::chrono::nanoseconds> const timeout = {},
            felspar::source_location const &loc =
                    felspar::source_location::current()) {
        return cnx.read_some(w, s, timeout, loc);
    }
    FELSPAR_CORO_WRAPPER inline auto write_some(
            warden &w,
            tls &cnx,
            std::span<std::byte const> const s,
            std::optional<std::chrono::nanoseconds> const timeout = {},
            felspar::source_location const &loc =
                    felspar::source_location::current()) {
        return cnx.write_some(w, s, timeout, loc);
    }


}
