#pragma once

#include <felspar/coro/starter.hpp>
#include <felspar/coro/stream.hpp>
#include <felspar/io/completion.hpp>
#include <felspar/io/deadline.hpp>
#include <felspar/io/pipe.hpp>
#include <felspar/io/posix.hpp>
#include <felspar/memory/pmr.hpp>

#include <chrono>
#include <span>


namespace felspar::io {


    class allocator;


    /// ## Warden
    /**
     * The warden is the primay abstraction of the library. It represents an
     * even loop on a single thread without committing to how that event loop is
     * implemented. It also acts an allocator which is able to allocate
     * coroutine frames.
     *
     * Because of this dual-use this `warden` type must remain only an
     * interface. Any attributes it has will break allocators as they will two
     * copies and no way to keep the attribute content the same.
     */
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


        /// ### Running coroutines

        /// #### Primary coroutine
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
        template<typename Ret, typename N, typename... PArgs, typename... MArgs>
        Ret run(N &o, task<Ret> (N::*f)(warden &, PArgs...), MArgs &&...margs) {
            auto task = (o.*f)(*this, std::forward<MArgs>(margs)...);
            auto handle = task.release();
            run_until(handle.get());
            return handle.promise().consume_value();
        }

        /// #### Single batch
        virtual void run_batch() = 0;
        /**
         * Run a single IO submission and resume ready coroutines. Only
         * processing is performed without any waits
         */

        /// #### Delayed resume
        virtual void async_resume(std::coroutine_handle<>) = 0;
        /**
         * Once the event loop has finished processing new events, then the
         * coroutines sent here will be resumed. This slight asynchrony can be
         * used to solve problems where a coroutine resume might destroy the
         * object that triggers the coroutine to be resumed.
         */


        /// ### File descriptors
        iop<void>
                close(socket_descriptor fd,
                      std::source_location const loc =
                              std::source_location::current()) {
            return do_close(fd, loc);
        }
        iop<void>
                close(posix::fd s,
                      std::source_location const loc =
                              std::source_location::current()) {
            return close(s.release(), loc);
        }


        /// ### Time management
        iop<void>
                sleep(std::chrono::nanoseconds ns,
                      std::source_location const loc =
                              std::source_location::current()) {
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
                std::optional<deadline> deadline = {},
                std::source_location const loc =
                        std::source_location::current()) {
            return do_read_some(fd, s, deadline, loc);
        }
        iop<std::size_t> read_some(
                socket_descriptor fd,
                std::span<std::byte> s,
                std::chrono::nanoseconds timeout,
                std::source_location const loc =
                        std::source_location::current()) {
            return read_some(fd, s, deadline_from(timeout), loc);
        }
        iop<std::size_t> read_some(
                posix::fd const &s,
                std::span<std::byte> b,
                std::optional<deadline> deadline = {},
                std::source_location const l = std::source_location::current()) {
            return read_some(s.native_handle(), b, deadline, l);
        }
        iop<std::size_t> read_some(
                posix::fd const &s,
                std::span<std::byte> b,
                std::chrono::nanoseconds timeout,
                std::source_location const l = std::source_location::current()) {
            return read_some(s.native_handle(), b, deadline_from(timeout), l);
        }
        iop<std::size_t> write_some(
                socket_descriptor fd,
                std::span<std::byte const> s,
                std::optional<deadline> deadline = {},
                std::source_location const loc =
                        std::source_location::current()) {
            return do_write_some(fd, s, deadline, loc);
        }
        iop<std::size_t> write_some(
                socket_descriptor fd,
                std::span<std::byte const> s,
                std::chrono::nanoseconds timeout,
                std::source_location const loc =
                        std::source_location::current()) {
            return write_some(fd, s, deadline_from(timeout), loc);
        }
        iop<std::size_t> write_some(
                posix::fd const &s,
                std::span<std::byte const> b,
                std::optional<deadline> deadline = {},
                std::source_location const l = std::source_location::current()) {
            return write_some(s.native_handle(), b, deadline, l);
        }
        iop<std::size_t> write_some(
                posix::fd const &s,
                std::span<std::byte const> b,
                std::chrono::nanoseconds timeout,
                std::source_location const l = std::source_location::current()) {
            return write_some(s.native_handle(), b, deadline_from(timeout), l);
        }


        /// ### Socket APIs

        /// #### Socket creation
        /**
         * Some wardens may need special socket options to be set, so in order
         * to be portable across wardens use this API instead of the POSIX
         * `::socket` one.
         */
        posix::fd create_socket(
                int domain,
                int type,
                int protocol,
                std::source_location = std::source_location::current());
        /// #### Create a TCP socket
        posix::fd create_tcp_socket(
                std::source_location const loc =
                        std::source_location::current()) {
            return create_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP, loc);
        }


        /// #### Pipe
        /**
         * Create a pipe usable by this warden. On Windows a true pipe will be
         * simulated by a socket pair.
         */
        pipe create_pipe(std::source_location = std::source_location::current());


        iop<socket_descriptor>
                accept(socket_descriptor fd,
                       std::optional<deadline> deadline = {},
                       std::source_location const loc =
                               std::source_location::current()) {
            return do_accept(fd, deadline, loc);
        }
        iop<socket_descriptor>
                accept(socket_descriptor fd,
                       std::chrono::nanoseconds timeout,
                       std::source_location const loc =
                               std::source_location::current()) {
            return accept(fd, deadline_from(timeout), loc);
        }
        iop<socket_descriptor>
                accept(posix::fd const &sock,
                       std::optional<deadline> deadline = {},
                       std::source_location const loc =
                               std::source_location::current()) {
            return accept(sock.native_handle(), deadline, loc);
        }
        iop<socket_descriptor>
                accept(posix::fd const &sock,
                       std::chrono::nanoseconds timeout,
                       std::source_location const loc =
                               std::source_location::current()) {
            return accept(sock.native_handle(), deadline_from(timeout), loc);
        }
        iop<void>
                connect(socket_descriptor fd,
                        sockaddr const *addr,
                        socklen_t addrlen,
                        std::optional<deadline> deadline = {},
                        std::source_location const loc =
                                std::source_location::current()) {
            return do_connect(fd, addr, addrlen, deadline, loc);
        }
        iop<void>
                connect(socket_descriptor fd,
                        sockaddr const *addr,
                        socklen_t addrlen,
                        std::chrono::nanoseconds timeout,
                        std::source_location const loc =
                                std::source_location::current()) {
            return connect(fd, addr, addrlen, deadline_from(timeout), loc);
        }
        iop<void>
                connect(posix::fd const &sock,
                        sockaddr const *addr,
                        socklen_t addrlen,
                        std::optional<deadline> deadline = {},
                        std::source_location const loc =
                                std::source_location::current()) {
            return connect(sock.native_handle(), addr, addrlen, deadline, loc);
        }
        iop<void>
                connect(posix::fd const &sock,
                        sockaddr const *addr,
                        socklen_t addrlen,
                        std::chrono::nanoseconds timeout,
                        std::source_location const loc =
                                std::source_location::current()) {
            return connect(
                    sock.native_handle(), addr, addrlen, deadline_from(timeout),
                    loc);
        }


        /// ### File readiness
        iop<void> read_ready(
                socket_descriptor fd,
                std::optional<deadline> deadline = {},
                std::source_location const loc =
                        std::source_location::current()) {
            return do_read_ready(fd, deadline, loc);
        }
        iop<void> read_ready(
                socket_descriptor fd,
                std::chrono::nanoseconds timeout,
                std::source_location const loc =
                        std::source_location::current()) {
            return read_ready(fd, deadline_from(timeout), loc);
        }
        iop<void> read_ready(
                posix::fd const &sock,
                std::optional<deadline> deadline = {},
                std::source_location const loc =
                        std::source_location::current()) {
            return read_ready(sock.native_handle(), deadline, loc);
        }
        iop<void> read_ready(
                posix::fd const &sock,
                std::chrono::nanoseconds timeout,
                std::source_location const loc =
                        std::source_location::current()) {
            return read_ready(
                    sock.native_handle(), deadline_from(timeout), loc);
        }
        iop<void> write_ready(
                socket_descriptor fd,
                std::optional<deadline> deadline = {},
                std::source_location const loc =
                        std::source_location::current()) {
            return do_write_ready(fd, deadline, loc);
        }
        iop<void> write_ready(
                socket_descriptor fd,
                std::chrono::nanoseconds timeout,
                std::source_location const loc =
                        std::source_location::current()) {
            return write_ready(fd, deadline_from(timeout), loc);
        }
        iop<void> write_ready(
                posix::fd const &sock,
                std::optional<deadline> deadline = {},
                std::source_location const loc =
                        std::source_location::current()) {
            return write_ready(sock.native_handle(), deadline, loc);
        }
        iop<void> write_ready(
                posix::fd const &sock,
                std::chrono::nanoseconds timeout,
                std::source_location const loc =
                        std::source_location::current()) {
            return write_ready(
                    sock.native_handle(), deadline_from(timeout), loc);
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
        virtual void run_until(std::coroutine_handle<>) = 0;
        virtual iop<void>
                do_close(socket_descriptor fd, std::source_location) = 0;
        virtual iop<void>
                do_sleep(std::chrono::nanoseconds, std::source_location) = 0;
        virtual iop<std::size_t> do_read_some(
                socket_descriptor fd,
                std::span<std::byte>,
                std::optional<deadline> timeout,
                std::source_location) = 0;
        virtual iop<std::size_t> do_write_some(
                socket_descriptor fd,
                std::span<std::byte const>,
                std::optional<deadline> timeout,
                std::source_location) = 0;
        virtual void do_prepare_socket(socket_descriptor, std::source_location) {
        }
        virtual iop<socket_descriptor> do_accept(
                socket_descriptor fd,
                std::optional<deadline> timeout,
                std::source_location) = 0;
        virtual iop<void> do_connect(
                socket_descriptor fd,
                sockaddr const *,
                socklen_t,
                std::optional<deadline> timeout,
                std::source_location) = 0;
        virtual iop<void> do_read_ready(
                socket_descriptor fd,
                std::optional<deadline> timeout,
                std::source_location) = 0;
        virtual iop<void> do_write_ready(
                socket_descriptor fd,
                std::optional<deadline> timeout,
                std::source_location) = 0;
    };


    template<typename R>
    inline iop<R>::~iop() {
        if (comp and comp->delete_due_to_iop_destructed()) { delete comp; }
    }


}
