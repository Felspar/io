#pragma once


#include <felspar/coro/task.hpp>

#include <memory>


namespace felspar::poll {


    template<typename R>
    class iop;
    class warden;


    struct completion {
        felspar::coro::coroutine_handle<> handle;
        int result = {};
        std::exception_ptr exception;

        virtual ~completion() = default;

        virtual warden *ward() = 0;
        virtual void await_suspend(felspar::coro::coroutine_handle<> h) {
            handle = h;
            try_or_resume();
        }
        virtual void try_or_resume() { handle.resume(); }
    };


    template<>
    class iop<void> {
      public:
        using result_type = void;

        iop(completion *c) : comp{c} {}
        ~iop();

        bool await_ready() const noexcept { return false; }
        void await_suspend(felspar::coro::coroutine_handle<> h) {
            comp->await_suspend(h);
        }
        auto await_resume() noexcept {
            if (comp->exception) { std::rethrow_exception(comp->exception); }
        }

      private:
        completion *comp;
    };


    template<typename R>
    class iop {
      public:
        using result_type = R;

        iop(completion *c) : comp{c} {}
        ~iop();

        bool await_ready() const noexcept { return false; }
        void await_suspend(felspar::coro::coroutine_handle<> h) {
            comp->await_suspend(h);
        }
        R await_resume() {
            if (comp->exception) {
                std::rethrow_exception(comp->exception);
            } else {
                return comp->result;
            }
        }

      private:
        completion *comp;
    };


}
