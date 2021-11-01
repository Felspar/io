#pragma once


#include <felspar/poll/warden.hpp>


namespace felspar::poll {


    class poll_warden : public warden {
        void run(felspar::coro::unique_handle<felspar::coro::task_promise<void>>);

        struct request {
            std::vector<felspar::coro::coroutine_handle<>> reads, writes;
        };
        std::map<int, request> requests;

      public:
        template<typename... PArgs, typename... MArgs>
        void run(coro::task<void> (*f)(warden &, PArgs...), MArgs &&...margs) {
            auto task = f(*this, std::forward<MArgs>(margs)...);
            run(task.release());
        }

        iop read(int fd) override;
        iop write(int fd) override;
    };


}
