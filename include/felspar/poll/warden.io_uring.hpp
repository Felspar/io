#pragma once


#include <felspar/poll/warden.hpp>


namespace felspar::poll {


    class io_uring_warden : public warden {
        void run_until(
                felspar::coro::unique_handle<felspar::coro::task_promise<void>>)
                override;
        void cancel(iop<void>::completion *) override;

      public:
        io_uring_warden();

        iop<int> accept(int fd) override;
        iop<void> connect(int fd, sockaddr const *, socklen_t) override;

        iop<void> read_ready(int fd) override;
        iop<void> write_ready(int fd) override;
    };


}
