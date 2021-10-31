#pragma once


#include <felspar/coro/task.hpp>

#include <map>
#include <vector>

#include <poll.h>


namespace felspar::poll {


    class warden;


    struct iop {
        warden &exec;
        int fd;
        bool read;

        bool await_ready() const noexcept { return false; }
        void await_suspend(felspar::coro::coroutine_handle<> h) noexcept;
        auto await_resume() noexcept {}
    };


    /// Executor that allows `poll` based asynchronous IO
    class warden {
        friend class iop;

        std::vector<
                felspar::coro::unique_handle<felspar::coro::task_promise<void>>>
                live;

        void run(felspar::coro::unique_handle<felspar::coro::task_promise<void>>);

        struct request {
            std::vector<felspar::coro::coroutine_handle<>> reads, writes;
        };
        std::map<int, request> requests;

      public:
        template<typename... PArgs, typename... MArgs>
        void post(coro::task<void> (*f)(warden &, PArgs...), MArgs &&...margs) {
            auto task = f(*this, std::forward<MArgs>(margs)...);
            auto coro = task.release();
            coro.resume();
            live.push_back(std::move(coro));
        }
        template<typename... PArgs, typename... MArgs>
        void run(coro::task<void> (*f)(warden &, PArgs...), MArgs &&...margs) {
            auto task = f(*this, std::forward<MArgs>(margs)...);
            run(task.release());
        }

        iop read(int fd);
        iop write(int fd);
    };


}
