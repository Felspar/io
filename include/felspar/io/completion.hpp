#pragma once


#include <felspar/coro/task.hpp>
#include <felspar/test/source.hpp>

#include <memory>


namespace felspar::io {


    class warden;


    template<typename R>
    struct completion {
        using result_type = R;

        std::size_t iop_count = 1;
        felspar::coro::coroutine_handle<> handle;
        felspar::source_location loc;
        R result = {};
        std::exception_ptr exception;

        completion(felspar::source_location l) : loc{std::move(l)} {}
        virtual ~completion() = default;

        virtual warden *ward() = 0;
        virtual felspar::coro::coroutine_handle<>
                await_suspend(felspar::coro::coroutine_handle<>) = 0;
    };
    template<>
    struct completion<void> {
        using result_type = void;

        std::size_t iop_count = 1;
        felspar::coro::coroutine_handle<> handle;
        felspar::source_location loc;
        std::exception_ptr exception;

        completion(felspar::source_location l) : loc{std::move(l)} {}
        virtual ~completion() = default;

        virtual warden *ward() = 0;
        virtual felspar::coro::coroutine_handle<>
                await_suspend(felspar::coro::coroutine_handle<> h) = 0;
    };


    template<typename R>
    class iop {
      public:
        using result_type = R;
        using completion_type = completion<result_type>;

        iop(completion_type *c) : comp{c} {}
        ~iop();

        bool await_ready() const noexcept { return false; }
        felspar::coro::coroutine_handle<>
                await_suspend(felspar::coro::coroutine_handle<> h) {
            return comp->await_suspend(h);
        }
        R await_resume() {
            if (comp->exception) {
                std::rethrow_exception(comp->exception);
            } else {
                return std::move(comp->result);
            }
        }

      private:
        completion_type *comp;
    };
    template<>
    class iop<void> {
      public:
        using result_type = void;
        using completion_type = completion<result_type>;

        iop(completion_type *c) : comp{c} {}
        ~iop();

        bool await_ready() const noexcept { return false; }
        felspar::coro::coroutine_handle<>
                await_suspend(felspar::coro::coroutine_handle<> h) {
            return comp->await_suspend(h);
        }
        auto await_resume() {
            if (comp->exception) { std::rethrow_exception(comp->exception); }
        }

      private:
        completion_type *comp;
    };


}
