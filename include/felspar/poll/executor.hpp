#pragma once


#include <felspar/coro/task.hpp>


namespace felspar::poll {


    /// Executor that allows `poll` based asynchronous IO
    class executor {
      public:
        coro::task<void> post(coro::task<void> (*)());
    };


}
