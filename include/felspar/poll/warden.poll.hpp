#pragma once


#include <felspar/poll/warden.hpp>

#include <map>
#include <vector>


namespace felspar::poll {


    class poll_warden : public warden {
        struct request {
            std::vector<completion *> reads, writes;
        };
        std::map<int, request> requests;

        void run_until(
                felspar::coro::unique_handle<felspar::coro::task_promise<void>>)
                override;
        void cancel(completion *) override;

      public:
        int create_socket(
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
