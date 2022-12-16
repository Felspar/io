#pragma once


#include <felspar/io/warden.hpp>


namespace felspar::io {


    class uring_warden : public warden {
        void run_until(felspar::coro::coroutine_handle<>) override;

        struct delivery;
        template<typename R>
        struct completion;
        struct impl;
        std::unique_ptr<impl> ring;

        struct close_completion;
        struct sleep_completion;
        struct read_some_completion;
        struct write_some_completion;
        struct accept_completion;
        struct connect_completion;
        struct poll_completion;

      public:
        uring_warden() : uring_warden{100, {}} {}
        explicit uring_warden(unsigned entries, unsigned flags = {});
        ~uring_warden();

      protected:
        /// File descriptors
        iop<void> do_close(int fd, felspar::source_location const &) override;

        /// Time management
        iop<void> do_sleep(
                std::chrono::nanoseconds,
                felspar::source_location const &) override;

        /// Read & write
        iop<std::size_t> do_read_some(
                socket_descriptor fd,
                std::span<std::byte>,
                std::optional<std::chrono::nanoseconds>,
                felspar::source_location const &) override;
        iop<std::size_t> do_write_some(
                socket_descriptor fd,
                std::span<std::byte const>,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) override;

        /// Sockets
        iop<socket_descriptor> do_accept(
                socket_descriptor fd,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) override;
        iop<void> do_connect(
                socket_descriptor fd,
                sockaddr const *,
                socklen_t,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) override;

        /// File descriptor readiness
        iop<void> do_read_ready(
                socket_descriptor fd,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) override;
        iop<void> do_write_ready(
                socket_descriptor fd,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) override;
    };


}
