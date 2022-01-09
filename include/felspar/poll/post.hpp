#pragma once


#include <felspar/coro/task.hpp>

#include <vector>


namespace felspar::poll {


    class warden;


    class coro_owner {
        std::vector<
                felspar::coro::unique_handle<felspar::coro::task_promise<void>>>
                live;

      public:
        coro_owner(warden &) {}

        template<typename... PArgs, typename... MArgs>
        void
                post(coro::task<void> (*f)(warden &, PArgs...),
                     warden &w,
                     MArgs &&...margs) {
            auto task = f(w, std::forward<MArgs>(margs)...);
            auto coro = task.release();
            coro.resume();
            live.push_back(std::move(coro));
        }

        /// Garbage collect old coroutines
        void gc() {
            /// TODO This implementation is not exception safe. If
            /// `consume_value` throws then the entire container is in a weird
            /// state
            live.erase(
                    std::remove_if(
                            live.begin(), live.end(),
                            [](auto const &h) {
                                if (h.done()) {
                                    h.promise().consume_value();
                                    return true;
                                } else {
                                    return false;
                                }
                            }),
                    live.end());
        }
    };


}
