#pragma once


#include <felspar/poll/warden.hpp>

#include <map>
#include <vector>


namespace felspar::poll {


    class poll_warden : public warden {
        struct request {
            std::vector<felspar::coro::coroutine_handle<>> reads, writes;
        };
        std::map<int, request> requests;

        void run_until(
                felspar::coro::unique_handle<felspar::coro::task_promise<void>>)
                override;
        void cancel(iop<void>::completion *) override;

      public:
        iop<int> accept(int fd) override;

        iop<void> read_ready(int fd) override;
        iop<void> write_ready(int fd) override;
    };


}
