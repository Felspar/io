#pragma once

#include <felspar/coro/task.hpp>
#include <felspar/io/exceptions.hpp>
#include <felspar/test/source.hpp>

#include <memory>
#include <system_error>


namespace felspar::io {


    class warden;


    template<typename R>
    struct completion {
        using result_type = R;

        std::size_t iop_count = 1;
        felspar::coro::coroutine_handle<> handle;
        R result = {};

        /// Error fields
        felspar::source_location loc;
        std::error_code error = {};
        char const *message = "";
        [[noreturn]] void throw_exception() {
            if (error == timeout::error) {
                throw timeout{message, std::move(loc)};
            } else {
                throw felspar::stdexcept::system_error{
                        error, message, std::move(loc)};
            }
        }

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

        /// Error fields
        felspar::source_location loc;
        std::error_code error = {};
        char const *message = "";
        [[noreturn]] void throw_exception() {
            if (error == std::error_code{ETIME, std::system_category()}) {
                throw timeout{message, std::move(loc)};
            } else {
                throw felspar::stdexcept::system_error{
                        error, message, std::move(loc)};
            }
        }

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
            if (comp->error) {
                comp->throw_exception();
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
            if (comp->error) { comp->throw_exception(); }
        }

      private:
        completion_type *comp;
    };


}
