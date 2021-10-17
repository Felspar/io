#pragma once


#include <felspar/coro/task.hpp>

#include <vector>


namespace felspar::poll {


    /// Executor that allows `poll` based asynchronous IO
    class executor {
        std::vector<
                felspar::coro::unique_handle<felspar::coro::task_promise<void>>>
                live;

      public:
        template<typename... PArgs, typename... MArgs>
        void
                post(coro::task<void> (*f)(executor &, PArgs...),
                     MArgs &&...margs) {
            auto task = f(*this, std::forward<MArgs>(margs)...);
            auto coro = task.release();
            coro.resume();
            live.push_back(std::move(coro));
        }
        template<typename... PArgs, typename... MArgs>
        void run(coro::task<void> (*f)(executor &, PArgs...), MArgs &&...margs) {
            auto task = f(*this, std::forward<MArgs>(margs)...);
            auto coro = task.release();
            coro.resume();
            while (not coro.done()) {
                /// Do poll here....
            }
        }
    };


}
