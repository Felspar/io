#pragma once


#include <felspar/io/socket_descriptor.hpp>
#include <felspar/exceptions/system_error.hpp>

#include <cstdint>

#include <unistd.h>


namespace felspar::posix {


    /// ## A very simple type for RAII management of a file descriptor
    class fd {
        io::socket_descriptor f;

        static void close_socket_if_valid(io::socket_descriptor const s) {
            if (s != io::invalid_handle) {
#ifdef FELSPAR_WINSOCK2
                ::closesocket(s);
#else
                ::close(s);
#endif
            }
        }

      public:
        fd() : f{io::invalid_handle} {}
        explicit fd(io::socket_descriptor f) : f{f} {}
        fd(fd &&o) : f{std::exchange(o.f, io::invalid_handle)} {}
        fd(fd const &) = delete;
        ~fd() { close_socket_if_valid(f); }

        fd &operator=(fd &&o) {
            fd s{std::exchange(f, std::exchange(o.f, io::invalid_handle))};
            return *this;
        }
        fd &operator=(fd const &) = delete;


        /// ### `true` if the file descriptor looks valid
        explicit operator bool() const noexcept {
            return f != io::invalid_handle;
        }
        io::socket_descriptor native_handle() const noexcept { return f; }


        /// ### Release the file descriptor
        io::socket_descriptor release() noexcept {
            return std::exchange(f, io::invalid_handle);
        }


        /// ### Close the FD
        /**
         * This performs a blocking `close`. You should use the warden's
         * asynchronous `close` if you can.
         */
        void close() noexcept {
            io::socket_descriptor c = std::exchange(f, io::invalid_handle);
            close_socket_if_valid(c);
        }
    };


    /// ## `select` handling
    /**
     * Set the number of open file descriptors this process can use to the
     * maximum allowed range. This is only set at a lower level due to the poor
     * design of the `select` system call, so if we promise never to use
     * `select` we can have a much higher number. Returns the old limit and the
     * new limit
     */
    std::pair<std::size_t, std::size_t> promise_to_never_use_select();


    /// ## Set the listen queue length for the socket
    void
            listen(io::socket_descriptor fd,
                   int backlog,
                   felspar::source_location const & =
                           felspar::source_location::current());
    inline void
            listen(posix::fd const &fd,
                   int backlog,
                   felspar::source_location const &loc =
                           felspar::source_location::current()) {
        listen(fd.native_handle(), backlog, loc);
    }


    /// ## Set a file descriptor to non-blocking mode
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


    /// ## Set a socket port for re-use
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


    /// ## Bind
    void
            bind(io::socket_descriptor sock,
                 std::uint32_t addr,
                 std::uint16_t port,
                 felspar::source_location const & =
                         felspar::source_location::current());
    inline void
            bind(fd const &sock,
                 std::uint32_t const addr,
                 std::uint16_t const port,
                 felspar::source_location const &loc =
                         felspar::source_location::current()) {
        posix::bind(sock.native_handle(), addr, port, loc);
    }
    inline void bind_to_any_address(
            io::socket_descriptor sock,
            std::uint16_t port,
            felspar::source_location const &loc =
                    felspar::source_location::current()) {
        posix::bind(sock, INADDR_ANY, port, loc);
    }
    inline void bind_to_any_address(
            fd const &sock,
            std::uint16_t const port,
            felspar::source_location const &loc =
                    felspar::source_location::current()) {
        posix::bind(sock.native_handle(), INADDR_ANY, port, loc);
    }


}
