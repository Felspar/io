#pragma once


#include <felspar/coro/task.hpp>
#include <felspar/test/source.hpp>

#include <memory>


namespace felspar::io {


    template<typename R>
    class iop;
    class warden;


    template<typename R>
    struct completion {
        using result_type = R;

        felspar::coro::coroutine_handle<> handle;
        felspar::source_location loc;
        R result = {};
        std::exception_ptr exception;

        completion(felspar::source_location l) : loc{std::move(l)} {}
        virtual ~completion() = default;

        virtual warden *ward() = 0;
        virtual void await_suspend(felspar::coro::coroutine_handle<>) = 0;
    };
    template<>
    struct completion<void> {
        using result_type = void;

        felspar::coro::coroutine_handle<> handle;
        felspar::source_location loc;
        std::exception_ptr exception;

        completion(felspar::source_location l) : loc{std::move(l)} {}
        virtual ~completion() = default;

        virtual warden *ward() = 0;
        virtual void await_suspend(felspar::coro::coroutine_handle<> h) = 0;
    };


    template<>
    class iop<void> {
      public:
        using result_type = void;
        using completion_type = completion<result_type>;

        iop(completion_type *c) : comp{c} {}
        ~iop();

        bool await_ready() const noexcept { return false; }
        void await_suspend(felspar::coro::coroutine_handle<> h) {
            comp->await_suspend(h);
        }
        auto await_resume() noexcept {
            if (comp->exception) { std::rethrow_exception(comp->exception); }
        }

      private:
        completion_type *comp;
    };


    template<typename R>
    class iop {
      public:
        using result_type = R;
        using completion_type = completion<result_type>;

        iop(completion_type *c) : comp{c} {}
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
        completion_type *comp;
    };


}
