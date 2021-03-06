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

      protected:
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
