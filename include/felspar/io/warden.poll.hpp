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

        void run_until(felspar::coro::coroutine_handle<>) override;

        struct read_some_completion;
        struct write_some_completion;
        struct accept_completion;
        struct connect_completion;
        struct read_ready_completion;
        struct write_ready_completion;

      public:
        /// Read & Write
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
        posix::fd create_socket(
                int domain,
                int type,
                int protocol,
                felspar::source_location =
                        felspar::source_location::current()) override;

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
