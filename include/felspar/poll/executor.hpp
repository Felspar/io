#pragma once


#include <felspar/coro/task.hpp>


namespace felspar::poll {


    /// Executor that allows `poll` based asynchronous IO
    class executor {
      public:
        template<typename... PArgs, typename... MArgs>
        void
                post(coro::task<void> (*f)(executor &, PArgs...),
                     MArgs &&...margs) {
            auto coro = f(*this, std::forward<MArgs>(margs)...);
        }
    };


}
