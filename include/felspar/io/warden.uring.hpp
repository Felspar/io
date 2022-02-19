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

        /// Time
        using warden::sleep;
        iop<void>
                sleep(std::chrono::nanoseconds,
                      felspar::source_location const & =
                              felspar::source_location::current()) override;

        /// Read & write
        using warden::read_some;
        iop<std::size_t> read_some(
                int fd,
                std::span<std::byte>,
                std::optional<std::chrono::nanoseconds> = {},
                felspar::source_location const & =
                        felspar::source_location::current()) override;
        using warden::write_some;
        iop<std::size_t> write_some(
                int fd,
                std::span<std::byte const>,
                std::optional<std::chrono::nanoseconds> timeout = {},
                felspar::source_location const & =
                        felspar::source_location::current()) override;

        /// Sockets
        using warden::accept;
        iop<int>
                accept(int fd,
                       std::optional<std::chrono::nanoseconds> timeout = {},
                       felspar::source_location const & =
                               felspar::source_location::current()) override;
        using warden::connect;
        iop<void>
                connect(int fd,
                        sockaddr const *,
                        socklen_t,
                        std::optional<std::chrono::nanoseconds> timeout = {},
                        felspar::source_location const & =
                                felspar::source_location::current()) override;

        /// File descriptor readiness
        using warden::read_ready;
        iop<void> read_ready(
                int fd,
                std::optional<std::chrono::nanoseconds> timeout = {},
                felspar::source_location const & =
                        felspar::source_location::current()) override;
        using warden::write_ready;
        iop<void> write_ready(
                int fd,
                std::optional<std::chrono::nanoseconds> timeout = {},
                felspar::source_location const & =
                        felspar::source_location::current()) override;
    };


}
