#pragma once


#include <felspar/coro/task.hpp>

#include <memory>


namespace felspar::poll {


    template<typename R>
    class iop;
    class warden;


    template<>
    class iop<void> {
      public:
        struct completion {
            warden *ward;

            completion(warden *w) : ward{w} {}
            virtual ~completion() = default;

            virtual void await_suspend(
                    felspar::coro::coroutine_handle<> h) noexcept = 0;
        };

        iop(completion *c) : comp{c} {}
        ~iop();

        bool await_ready() const noexcept { return false; }
        void await_suspend(felspar::coro::coroutine_handle<> h) noexcept {
            comp->await_suspend(h);
        }
        auto await_resume() noexcept {}

      private:
        completion *comp;
    };


    template<typename R>
    class iop {};


}
