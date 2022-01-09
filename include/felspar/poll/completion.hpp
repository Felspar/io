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
        using result_type = void;

        struct completion {
            warden *ward;
            felspar::coro::coroutine_handle<> handle;

            completion(warden *w) : ward{w} {}
            virtual ~completion() = default;

            virtual void await_suspend(felspar::coro::coroutine_handle<> h) {
                handle = h;
                try_or_resume();
            }
            virtual void try_or_resume() { handle.resume(); }
        };

        iop(completion *c) : comp{c} {}
        ~iop();

        bool await_ready() const noexcept { return false; }
        void await_suspend(felspar::coro::coroutine_handle<> h) {
            comp->await_suspend(h);
        }
        auto await_resume() noexcept {}

      private:
        completion *comp;
    };


    template<typename R>
    class iop {
      public:
        using result_type = R;

        struct completion : public iop<void>::completion {
            result_type result;
            completion(warden *w) : iop<void>::completion(w) {}
        };

        iop(completion *c) : comp{c} {}
        ~iop();

        bool await_ready() const noexcept { return false; }
        void await_suspend(felspar::coro::coroutine_handle<> h) {
            comp->await_suspend(h);
        }
        R await_resume() { return comp->result; }

      private:
        completion *comp;
    };


}
