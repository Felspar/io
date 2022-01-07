#pragma once


#include <felspar/coro/task.hpp>

#include <functional>
#include <map>
#include <vector>

#include <poll.h>


namespace felspar::poll {


    template<typename R>
    struct iop;
    class warden;



    template<>
    struct iop<void> {
        /// TODO Want something with much lower overhead than std::function
        std::function<void(felspar::coro::coroutine_handle<>)> suspend;

        bool await_ready() const noexcept { return false; }
        void await_suspend(felspar::coro::coroutine_handle<> h) noexcept {
            suspend(h);
        }
        auto await_resume() noexcept {}
    };


    /// Executor that allows `poll` based asynchronous IO
    class warden {
      protected:
        std::vector<
                felspar::coro::unique_handle<felspar::coro::task_promise<void>>>
                live;

      public:
        template<typename... PArgs, typename... MArgs>
        void post(coro::task<void> (*f)(warden &, PArgs...), MArgs &&...margs) {
            auto task = f(*this, std::forward<MArgs>(margs)...);
            auto coro = task.release();
            coro.resume();
            live.push_back(std::move(coro));
        }

        virtual iop<void> read_ready(int fd) = 0;
        virtual iop<void> write_ready(int fd) = 0;
    };


}
