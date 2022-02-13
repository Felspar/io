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

        struct sleep_completion;
        struct read_some_completion;
        struct write_some_completion;
        struct accept_completion;
        struct connect_completion;
        struct read_ready_completion;
        struct write_ready_completion;

      public:
        /// Time
        using warden::sleep;
        iop<void>
                sleep(std::chrono::nanoseconds,
                      felspar::source_location =
                              felspar::source_location::current()) override;

        /// Read & Write
        using warden::read_some;
        iop<std::size_t> read_some(
                int fd,
                std::span<std::byte>,
                std::optional<std::chrono::nanoseconds> = {},
                felspar::source_location =
                        felspar::source_location::current()) override;
        using warden::write_some;
        iop<std::size_t> write_some(
                int fd,
                std::span<std::byte const>,
                std::optional<std::chrono::nanoseconds> timeout = {},
                felspar::source_location =
                        felspar::source_location::current()) override;

        /// Sockets
        posix::fd create_socket(
                int domain,
                int type,
                int protocol,
                felspar::source_location =
                        felspar::source_location::current()) override;

        using warden::accept;
        iop<int>
                accept(int fd,
                       std::optional<std::chrono::nanoseconds> timeout = {},
                       felspar::source_location =
                               felspar::source_location::current()) override;
        using warden::connect;
        iop<void>
                connect(int fd,
                        sockaddr const *,
                        socklen_t,
                        felspar::source_location =
                                felspar::source_location::current()) override;

        /// File descriptor readiness
        using warden::read_ready;
        iop<void> read_ready(
                int fd,
                felspar::source_location =
                        felspar::source_location::current()) override;
        using warden::write_ready;
        iop<void> write_ready(
                int fd,
                felspar::source_location =
                        felspar::source_location::current()) override;
    };


}
