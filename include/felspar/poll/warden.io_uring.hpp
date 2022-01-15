#pragma once


#include <felspar/poll/warden.hpp>


namespace felspar::poll {


    class io_uring_warden : public warden {
        void run_until(
                felspar::coro::unique_handle<felspar::coro::task_promise<void>>)
                override;
        void cancel(poll::completion *) override;

        struct impl;
        std::unique_ptr<impl> ring;

      public:
        io_uring_warden() : io_uring_warden{100, {}} {}
        explicit io_uring_warden(unsigned entries, unsigned flags = {});
        ~io_uring_warden();

        iop<int> accept(int fd) override;
        iop<void> connect(int fd, sockaddr const *, socklen_t) override;

        iop<void> read_ready(int fd) override;
        iop<void> write_ready(int fd) override;

        struct completion;
    };


}
