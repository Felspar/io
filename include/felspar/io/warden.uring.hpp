#pragma once


#include <felspar/io/async_resumption.hpp>
#include <felspar/io/warden.hpp>


namespace felspar::io {


    class uring_warden : public warden {
        void run_until(std::coroutine_handle<>) override;
        void do_async_resume(std::span<std::coroutine_handle<> const>) override;
        void wake_event_loop();

        struct delivery;
        template<typename R>
        struct completion;
        struct impl;
        std::unique_ptr<impl> ring;
        async_resumption resumer;

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

        void run_batch() override;

      protected:
        /// File descriptors
        iop<void> do_close(socket_descriptor fd, std::source_location) override;

        /// Time management
        iop<void> do_sleep(
                std::chrono::nanoseconds, std::source_location) override;

        /// Read & write
        iop<std::size_t> do_read_some(
                socket_descriptor fd,
                std::span<std::byte>,
                std::optional<deadline>,
                std::source_location) override;
        iop<std::size_t> do_write_some(
                socket_descriptor fd,
                std::span<std::byte const>,
                std::optional<deadline> timeout,
                std::source_location) override;

        /// Sockets
        iop<socket_descriptor> do_accept(
                socket_descriptor fd,
                std::optional<deadline> timeout,
                std::source_location) override;
        iop<void> do_connect(
                socket_descriptor fd,
                sockaddr const *,
                socklen_t,
                std::optional<deadline> timeout,
                std::source_location) override;

        /// File descriptor readiness
        iop<void> do_read_ready(
                socket_descriptor fd,
                std::optional<deadline> timeout,
                std::source_location) override;
        iop<void> do_write_ready(
                socket_descriptor fd,
                std::optional<deadline> timeout,
                std::source_location) override;
    };


}
