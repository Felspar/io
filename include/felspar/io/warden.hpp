#pragma once

#include <felspar/io/completion.hpp>
#include <felspar/io/posix.hpp>
#include <felspar/test/source.hpp>

#include <chrono>
#include <span>

#include <netinet/in.h>


namespace felspar::io {


    class warden {
        template<typename R>
        friend struct felspar::io::iop;

      public:
        virtual ~warden() = default;

        template<typename Ret, typename... PArgs, typename... MArgs>
        Ret run(coro::task<Ret> (*f)(warden &, PArgs...), MArgs &&...margs) {
            auto task = f(*this, std::forward<MArgs>(margs)...);
            auto handle = task.release();
            run_until(handle.get());
            return handle.promise().consume_value();
        }

        /**
         * ### File descriptors
         */
        iop<void>
                close(int fd,
                      felspar::source_location const &loc =
                              felspar::source_location::current()) {
            return do_close(fd, loc);
        }
        iop<void>
                close(posix::fd s,
                      felspar::source_location const &loc =
                              felspar::source_location::current()) {
            return close(s.release(), loc);
        }

        /**
         * ### Time management
         */
        iop<void>
                sleep(std::chrono::nanoseconds ns,
                      felspar::source_location const &loc =
                              felspar::source_location::current()) {
            return do_sleep(ns, loc);
        }

        /**
         * ### Reading and writing
         */
        /// Read or write bytes from the provided buffer returning the number of
        /// bytes read/written.
        iop<std::size_t> read_some(
                int fd,
                std::span<std::byte> s,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &loc =
                        felspar::source_location::current()) {
            return do_read_some(fd, s, timeout, loc);
        }
        iop<std::size_t> read_some(
                posix::fd const &s,
                std::span<std::byte> b,
                std::optional<std::chrono::nanoseconds> timeout = {},
                felspar::source_location const &l =
                        felspar::source_location::current()) {
            return read_some(s.native_handle(), b, timeout, l);
        }
        iop<std::size_t> write_some(
                int fd,
                std::span<std::byte const> s,
                std::optional<std::chrono::nanoseconds> timeout = {},
                felspar::source_location const &loc =
                        felspar::source_location::current()) {
            return do_write_some(fd, s, timeout, loc);
        }
        iop<std::size_t> write_some(
                posix::fd const &s,
                std::span<std::byte const> b,
                std::optional<std::chrono::nanoseconds> timeout = {},
                felspar::source_location const &l =
                        felspar::source_location::current()) {
            return write_some(s.native_handle(), b, timeout, l);
        }

        /**
         * ### Socket APIs
         */
        /// Some wardens may need special socket options to be set, so in order
        /// to be portable across wardens use this API instead of the POSIX
        /// `::socket` one.
        posix::fd create_socket(
                int domain,
                int type,
                int protocol,
                felspar::source_location const & =
                        felspar::source_location::current());
        /// Create a pipe that has also had its end points wrapped by this warden.
        posix::pipe create_pipe(
                felspar::source_location const & =
                        felspar::source_location::current());


        iop<int>
                accept(int fd,
                       std::optional<std::chrono::nanoseconds> timeout = {},
                       felspar::source_location const &loc =
                               felspar::source_location::current()) {
            return do_accept(fd, timeout, loc);
        }
        iop<int>
                accept(posix::fd const &sock,
                       std::optional<std::chrono::nanoseconds> timeout = {},
                       felspar::source_location const &loc =
                               felspar::source_location::current()) {
            return accept(sock.native_handle(), timeout, loc);
        }
        iop<void>
                connect(int fd,
                        sockaddr const *addr,
                        socklen_t addrlen,
                        std::optional<std::chrono::nanoseconds> timeout = {},
                        felspar::source_location const &loc =
                                felspar::source_location::current()) {
            return do_connect(fd, addr, addrlen, timeout, loc);
        }
        iop<void>
                connect(posix::fd const &sock,
                        sockaddr const *addr,
                        socklen_t addrlen,
                        std::optional<std::chrono::nanoseconds> timeout = {},
                        felspar::source_location const &loc =
                                felspar::source_location::current()) {
            return connect(sock.native_handle(), addr, addrlen, timeout, loc);
        }


        /**
         * ### File readiness
         */
        iop<void> read_ready(
                int fd,
                std::optional<std::chrono::nanoseconds> timeout = {},
                felspar::source_location const &loc =
                        felspar::source_location::current()) {
            return do_read_ready(fd, timeout, loc);
        }
        iop<void> read_ready(
                posix::fd const &sock,
                std::optional<std::chrono::nanoseconds> timeout = {},
                felspar::source_location const &loc =
                        felspar::source_location::current()) {
            return read_ready(sock.native_handle(), timeout, loc);
        }
        iop<void> write_ready(
                int fd,
                std::optional<std::chrono::nanoseconds> timeout = {},
                felspar::source_location const &loc =
                        felspar::source_location::current()) {
            return do_write_ready(fd, timeout, loc);
        }
        iop<void> write_ready(
                posix::fd const &sock,
                std::optional<std::chrono::nanoseconds> timeout = {},
                felspar::source_location const &loc =
                        felspar::source_location::current()) {
            return write_ready(sock.native_handle(), timeout, loc);
        }

      protected:
        virtual void run_until(felspar::coro::coroutine_handle<>) = 0;
        virtual iop<void> do_close(int fd, felspar::source_location const &) = 0;
        virtual iop<void> do_sleep(
                std::chrono::nanoseconds, felspar::source_location const &) = 0;
        virtual iop<std::size_t> do_read_some(
                int fd,
                std::span<std::byte>,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) = 0;
        virtual iop<std::size_t> do_write_some(
                int fd,
                std::span<std::byte const>,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) = 0;
        virtual void
                do_prepare_socket(int sock, felspar::source_location const &) {}
        virtual iop<int> do_accept(
                int fd,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) = 0;
        virtual iop<void> do_connect(
                int fd,
                sockaddr const *,
                socklen_t,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) = 0;
        virtual iop<void> do_read_ready(
                int fd,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) = 0;
        virtual iop<void> do_write_ready(
                int fd,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) = 0;
    };


    template<typename R>
    inline iop<R>::~iop() {
        if (comp and comp->delete_due_to_iop_destructed()) { delete comp; }
    }


}
