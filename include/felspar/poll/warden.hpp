#pragma once

#include <felspar/poll/completion.hpp>


namespace felspar::poll {


    /// Executor that allows `poll` based asynchronous IO
    class warden {
        template<typename R>
        friend struct felspar::poll::iop;

      protected:
        virtual void run_until(felspar::coro::unique_handle<
                               felspar::coro::task_promise<void>>) = 0;
        virtual void cancel(iop<void>::completion *) = 0;

      public:
        virtual ~warden() = default;

        template<typename... PArgs, typename... MArgs>
        void run(coro::task<void> (*f)(warden &, PArgs...), MArgs &&...margs) {
            auto task = f(*this, std::forward<MArgs>(margs)...);
            run_until(task.release());
        }

        /**
         * ### Socket APIs
         */
        //         virtual iop<int> accept(int fd) = 0;

        /**
         * ### File readiness
         */
        virtual iop<void> read_ready(int fd) = 0;
        virtual iop<void> write_ready(int fd) = 0;
    };


    inline iop<void>::~iop() { comp->ward->cancel(comp); }


}
