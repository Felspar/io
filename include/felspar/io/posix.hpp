#pragma once


#include <felspar/io/socket_descriptor.hpp>
#include <felspar/exceptions/system_error.hpp>

#include <unistd.h>


namespace felspar::posix {


    /// ## A very simple type for RAII management of a file descriptor
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


        /// ### `true` if the file descriptor looks valid
        explicit operator bool() const noexcept { return f >= 0; }
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
            if (c >= 0) { ::close(c); }
        }
    };


    /// ## A pipe has a read and a write end
    struct pipe {
        fd read, write;

        /// ### Create a new pipe
        /// The file descriptors will be set to non-blocking mode.
        static pipe create();
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


    /// ## Bind to any address
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
