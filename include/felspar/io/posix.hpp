#pragma once


#include <felspar/exceptions/system_error.hpp>

#include <unistd.h>

#if __has_include(<netinet/in.h>)

/// Standard POSIX system
#define FELSPAR_POSIX_SOCKETS
#include <netinet/in.h>

#elif __has_include(<winsock2.h>)

/// Windows POSIX like system
#define FELSPAR_WINSOCK2
#include <winsock2.h>
using socklen_t = int;

#endif


namespace felspar::posix {


    /// A very simple type for RAII management of a file descriptor
    class fd {
        int f;

      public:
        fd() : f{-1} {}
        explicit fd(int f) : f{f} {}
        fd(fd &&o) : f{std::exchange(o.f, -1)} {}
        fd(fd const &) = delete;
        ~fd() {
            if (f >= 0) { ::close(f); }
        }

        fd &operator=(fd &&o) {
            fd s{std::exchange(f, std::exchange(o.f, -1))};
            return *this;
        }
        fd &operator=(fd const &) = delete;

        /// `true` if the file descriptor looks valid
        explicit operator bool() const noexcept { return f >= 0; }
        int native_handle() const noexcept { return f; }

        int release() noexcept { return std::exchange(f, -1); }

        /// Close the FD
        void close() noexcept {
            int c = std::exchange(f, -1);
            if (c >= 0) { ::close(c); }
        }
    };


    /// Set a file descriptor to non-blocking mode
    void set_non_blocking(
            int sock,
            felspar::source_location const & =
                    felspar::source_location::current());
    inline void set_non_blocking(
            fd const &sock,
            felspar::source_location const &loc =
                    felspar::source_location::current()) {
        return set_non_blocking(sock.native_handle(), loc);
    }


    /// Set a socket port for re-use
    void set_reuse_port(
            int sock,
            felspar::source_location const & =
                    felspar::source_location::current());
    inline void set_reuse_port(
            fd const &sock,
            felspar::source_location const &loc =
                    felspar::source_location::current()) {
        return set_reuse_port(sock.native_handle(), loc);
    }


    /// Bind to any address
    void bind_to_any_address(
            int sock,
            std::uint16_t port,
            felspar::source_location const & =
                    felspar::source_location::current());
    inline void bind_to_any_address(
            fd const &sock,
            std::uint16_t const port,
            felspar::source_location const &loc =
                    felspar::source_location::current()) {
        return bind_to_any_address(sock.native_handle(), port, loc);
    }


}
