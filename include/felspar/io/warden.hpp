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

      protected:
        virtual void run_until(felspar::coro::coroutine_handle<>) = 0;
        template<typename R>
        void cancel(completion<R> *c) {
            if (--c->iop_count == 0) { delete c; }
        }

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
         * ### Time management
         */
        virtual iop<void>
                sleep(std::chrono::nanoseconds,
                      felspar::source_location const & =
                              felspar::source_location::current()) = 0;

        /**
         * ### Reading and writing
         */
        /// Read or write bytes from the provided buffer returning the number of
        /// bytes read/written.
        virtual iop<std::size_t> read_some(
                int fd,
                std::span<std::byte>,
                std::optional<std::chrono::nanoseconds> timeout = {},
                felspar::source_location const & =
                        felspar::source_location::current()) = 0;
        iop<std::size_t> read_some(
                posix::fd const &s,
                std::span<std::byte> b,
                std::optional<std::chrono::nanoseconds> timeout = {},
                felspar::source_location const &l =
                        felspar::source_location::current()) {
            return read_some(s.native_handle(), b, timeout, l);
        }
        virtual iop<std::size_t> write_some(
                int fd,
                std::span<std::byte const>,
                std::optional<std::chrono::nanoseconds> timeout = {},
                felspar::source_location const & =
                        felspar::source_location::current()) = 0;
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
        virtual posix::fd create_socket(
                int domain,
                int type,
                int protocol,
                felspar::source_location const & =
                        felspar::source_location::current());

        virtual iop<int>
                accept(int fd,
                       std::optional<std::chrono::nanoseconds> timeout = {},
                       felspar::source_location const & =
                               felspar::source_location::current()) = 0;
        iop<int>
                accept(posix::fd const &sock,
                       std::optional<std::chrono::nanoseconds> timeout = {},
                       felspar::source_location const &loc =
                               felspar::source_location::current()) {
            return accept(sock.native_handle(), timeout, loc);
        }
        virtual iop<void>
                connect(int fd,
                        sockaddr const *,
                        socklen_t,
                        std::optional<std::chrono::nanoseconds> timeout = {},
                        felspar::source_location const & =
                                felspar::source_location::current()) = 0;
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
        virtual iop<void> read_ready(
                int fd,
                std::optional<std::chrono::nanoseconds> timeout = {},
                felspar::source_location const & =
                        felspar::source_location::current()) = 0;
        iop<void> read_ready(
                posix::fd const &sock,
                std::optional<std::chrono::nanoseconds> timeout = {},
                felspar::source_location const &loc =
                        felspar::source_location::current()) {
            return read_ready(sock.native_handle(), timeout, loc);
        }
        virtual iop<void> write_ready(
                int fd,
                std::optional<std::chrono::nanoseconds> timeout = {},
                felspar::source_location const & =
                        felspar::source_location::current()) = 0;
        iop<void> write_ready(
                posix::fd const &sock,
                std::optional<std::chrono::nanoseconds> timeout = {},
                felspar::source_location const &loc =
                        felspar::source_location::current()) {
            return write_ready(sock.native_handle(), timeout, loc);
        }
    };


    template<typename R>
    inline iop<R>::~iop() {
        if (comp) { comp->ward()->cancel(comp); }
    }


}
