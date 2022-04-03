#pragma once


#include <felspar/io/warden.hpp>

#include <map>
#include <vector>


namespace felspar::io {


    class poll_warden : public warden {
        struct retrier;
        template<typename R>
        struct completion;
        struct request {
            std::vector<retrier *> reads, writes;
        };
        std::map<int, request> requests;
        std::multimap<std::chrono::steady_clock::time_point, retrier *> timeouts;

        void run_until(felspar::coro::coroutine_handle<>) override;

        struct close_completion;
        struct sleep_completion;
        struct read_some_completion;
        struct write_some_completion;
        struct accept_completion;
        struct connect_completion;
        struct read_ready_completion;
        struct write_ready_completion;

      protected:
        /// File descriptors
        iop<void> do_close(int fd, felspar::source_location const &) override;

        /// Time management
        iop<void> do_sleep(
                std::chrono::nanoseconds,
                felspar::source_location const &) override;

        /// Read & write
        iop<std::size_t> do_read_some(
                int fd,
                std::span<std::byte>,
                std::optional<std::chrono::nanoseconds>,
                felspar::source_location const &) override;
        iop<std::size_t> do_write_some(
                int fd,
                std::span<std::byte const>,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) override;

        /// Sockets
        void do_prepare_socket(
                int sock, felspar::source_location const &) override;
        iop<int> do_accept(
                int fd,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) override;
        iop<void> do_connect(
                int fd,
                sockaddr const *,
                socklen_t,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) override;

        /// File descriptor readiness
        iop<void> do_read_ready(
                int fd,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) override;
        iop<void> do_write_ready(
                int fd,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) override;
    };


}
