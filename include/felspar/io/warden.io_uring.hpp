#pragma once


#include <felspar/io/warden.hpp>


namespace felspar::io {


    class io_uring_warden : public warden {
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
        io_uring_warden() : io_uring_warden{100, {}} {}
        explicit io_uring_warden(unsigned entries, unsigned flags = {});
        ~io_uring_warden();

        /// Time
        iop<void>
                sleep(std::chrono::nanoseconds,
                      felspar::source_location =
                              felspar::source_location::current()) override;

        /// Read & write
        iop<std::size_t> read_some(
                int fd,
                std::span<std::byte>,
                felspar::source_location =
                        felspar::source_location::current()) override;
        iop<std::size_t> write_some(
                int fd,
                std::span<std::byte const>,
                felspar::source_location =
                        felspar::source_location::current()) override;

        /// Sockets
        iop<int>
                accept(int fd,
                       felspar::source_location =
                               felspar::source_location::current()) override;
        iop<void>
                connect(int fd,
                        sockaddr const *,
                        socklen_t,
                        felspar::source_location =
                                felspar::source_location::current()) override;

        /// File descriptor readiness
        iop<void> read_ready(
                int fd,
                felspar::source_location =
                        felspar::source_location::current()) override;
        iop<void> write_ready(
                int fd,
                felspar::source_location =
                        felspar::source_location::current()) override;
    };


}
