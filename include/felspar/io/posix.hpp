#pragma once


#include <felspar/io/socket_descriptor.hpp>
#include <felspar/exceptions/system_error.hpp>

#include <unistd.h>


namespace felspar::posix {


    /// A very simple type for RAII management of a file descriptor
    class fd {
        io::socket_descriptor f;

      public:
        fd() : f{io::invalid_handle} {}
        explicit fd(io::socket_descriptor f) : f{f} {}
        fd(fd &&o) : f{std::exchange(o.f, io::invalid_handle)} {}
        fd(fd const &) = delete;
        ~fd() {
            if (f >= 0) { ::close(f); }
        }

        fd &operator=(fd &&o) {
            fd s{std::exchange(f, std::exchange(o.f, io::invalid_handle))};
            return *this;
        }
        fd &operator=(fd const &) = delete;

        /// `true` if the file descriptor looks valid
        explicit operator bool() const noexcept { return f >= 0; }
        io::socket_descriptor native_handle() const noexcept { return f; }

        io::socket_descriptor release() noexcept {
            return std::exchange(f, io::invalid_handle);
        }

        /// Close the FD
        void close() noexcept {
            io::socket_descriptor c = std::exchange(f, io::invalid_handle);
            if (c >= 0) { ::close(c); }
        }
    };


    /// Set a file descriptor to non-blocking mode
    void set_non_blocking(
            io::socket_descriptor sock,
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
            io::socket_descriptor sock,
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
            io::socket_descriptor sock,
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
