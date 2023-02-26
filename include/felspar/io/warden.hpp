#pragma once

#include <felspar/coro/start.hpp>
#include <felspar/coro/stream.hpp>
#include <felspar/io/completion.hpp>
#include <felspar/io/posix.hpp>
#include <felspar/memory/pmr.hpp>
#include <felspar/test/source.hpp>

#include <chrono>
#include <span>


namespace felspar::io {


    class allocator;


    class warden : public felspar::pmr::memory_resource {
        friend class allocator;
        template<typename R>
        friend class felspar::io::iop;

      public:
        virtual ~warden() = default;

        template<typename R>
        using task = coro::task<R, warden>;
        template<typename R>
        using stream = coro::stream<R, warden>;

        template<typename R = void>
        using eager = coro::eager<task<R>>;
        template<typename R = void>
        using starter = coro::starter<task<R>>;

        template<typename Ret, typename... PArgs, typename... MArgs>
        Ret run(task<Ret> (*f)(warden &, PArgs...), MArgs &&...margs) {
            auto handle = f(*this, std::forward<MArgs>(margs)...).release();
            run_until(handle.get());
            return handle.promise().consume_value();
        }
        template<typename Ret, typename N, typename... PArgs, typename... MArgs>
        Ret run(N &o,
                coro::task<Ret> (N::*f)(warden &, PArgs...),
                MArgs &&...margs) {
            auto task = (o.*f)(*this, std::forward<MArgs>(margs)...);
            auto handle = task.release();
            run_until(handle.get());
            return handle.promise().consume_value();
        }

        /// Run a single IO submission and resume ready coroutines. Only
        /// processing is performed without any waits
        virtual void run_batch() = 0;

        /// ### File descriptors
        iop<void>
                close(socket_descriptor fd,
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

        /// ### Time management
        iop<void>
                sleep(std::chrono::nanoseconds ns,
                      felspar::source_location const &loc =
                              felspar::source_location::current()) {
            return do_sleep(ns, loc);
        }

        /// ### Reading and writing
        /**
         * Read or write bytes from the provided buffer returning the number of
         * bytes read/written.
         */
        iop<std::size_t> read_some(
                socket_descriptor fd,
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
                socket_descriptor fd,
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

        /// ### Socket APIs
        /**
         * Some wardens may need special socket options to be set, so in order
         * to be portable across wardens use this API instead of the POSIX
         * `::socket` one.
         */
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


        iop<socket_descriptor>
                accept(socket_descriptor fd,
                       std::optional<std::chrono::nanoseconds> timeout = {},
                       felspar::source_location const &loc =
                               felspar::source_location::current()) {
            return do_accept(fd, timeout, loc);
        }
        iop<socket_descriptor>
                accept(posix::fd const &sock,
                       std::optional<std::chrono::nanoseconds> timeout = {},
                       felspar::source_location const &loc =
                               felspar::source_location::current()) {
            return accept(sock.native_handle(), timeout, loc);
        }
        iop<void>
                connect(socket_descriptor fd,
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


        /// ### File readiness
        iop<void> read_ready(
                socket_descriptor fd,
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
                socket_descriptor fd,
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

      private:
        /// ### PMR based memory allocation
        void *do_allocate(
                std::size_t const bytes, std::size_t const alignment) override {
            return felspar::pmr::new_delete_resource()->allocate(
                    bytes, alignment);
        }
        void do_deallocate(
                void *const p,
                std::size_t const bytes,
                std::size_t const alignment) override {
            return felspar::pmr::new_delete_resource()->deallocate(
                    p, bytes, alignment);
        }
        bool do_is_equal(memory_resource const &other) const noexcept override {
            return this == &other;
        }


      protected:
        virtual void run_until(felspar::coro::coroutine_handle<>) = 0;
        virtual iop<void> do_close(
                socket_descriptor fd, felspar::source_location const &) = 0;
        virtual iop<void> do_sleep(
                std::chrono::nanoseconds, felspar::source_location const &) = 0;
        virtual iop<std::size_t> do_read_some(
                socket_descriptor fd,
                std::span<std::byte>,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) = 0;
        virtual iop<std::size_t> do_write_some(
                socket_descriptor fd,
                std::span<std::byte const>,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) = 0;
        virtual void do_prepare_socket(
                socket_descriptor, felspar::source_location const &) {}
        virtual iop<socket_descriptor> do_accept(
                socket_descriptor fd,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) = 0;
        virtual iop<void> do_connect(
                socket_descriptor fd,
                sockaddr const *,
                socklen_t,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) = 0;
        virtual iop<void> do_read_ready(
                socket_descriptor fd,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) = 0;
        virtual iop<void> do_write_ready(
                socket_descriptor fd,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) = 0;
    };


    template<typename R>
    inline iop<R>::~iop() {
        if (comp and comp->delete_due_to_iop_destructed()) { delete comp; }
    }


}
