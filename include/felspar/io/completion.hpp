#pragma once

#include <felspar/coro/task.hpp>
#include <felspar/io/exceptions.hpp>
#include <felspar/test/source.hpp>

#include <memory>
#include <system_error>


namespace felspar::io {


    template<typename R>
    class ec;
    class warden;


    template<typename R>
    struct outcome;
    template<>
    struct outcome<void> {
        std::error_code error = {};
        char const *message = "";
        [[noreturn]] void throw_exception(
                felspar::source_location loc =
                        felspar::source_location::current()) {
            if (error == timeout::error) {
                throw timeout{message, loc};
            } else {
                throw felspar::stdexcept::system_error{error, message, loc};
            }
        }
        explicit operator bool() const noexcept {
            return not static_cast<bool>(error);
        }
    };
    template<typename R>
    struct outcome : private outcome<void> {
        using outcome<void>::error;
        using outcome<void>::message;
        using outcome<void>::throw_exception;
        using outcome<void>::operator bool;

        std::optional<R> result = {};

        outcome &operator=(R r) {
            result = std::move(r);
            return *this;
        }
        R const &operator()() const { return result.value(); }
    };


    template<typename R>
    struct completion {
        using result_type = R;

        std::size_t iop_count = 1;
        felspar::coro::coroutine_handle<> handle = {};
        felspar::source_location loc;
        outcome<R> result = {};

        completion(felspar::source_location l) : loc{l} {}
        virtual ~completion() = default;

        virtual warden *ward() = 0;
        virtual felspar::coro::coroutine_handle<>
                await_suspend(felspar::coro::coroutine_handle<>) = 0;
    };


    template<typename R>
    class iop {
        template<typename E>
        friend class ec;

      public:
        using result_type = R;
        using completion_type = completion<result_type>;

        iop(completion_type *c) : comp{c} {}
        ~iop();

        iop(iop const &) = delete;
        iop &operator=(iop const &) = delete;
        iop(iop &&i) : comp{std::exchange(i.comp, {})} {}
        iop &operator=(iop &&i) {
            std::swap(i.comp, comp);
            return *this;
        }

        bool await_ready() const noexcept { return false; }
        felspar::coro::coroutine_handle<>
                await_suspend(felspar::coro::coroutine_handle<> h) {
            return comp->await_suspend(h);
        }
        R await_resume() {
            if (comp->result.error) {
                comp->result.throw_exception(comp->loc);
            } else {
                return std::move(comp->result.result.value());
            }
        }

      private:
        completion_type *comp;
    };
    template<>
    class iop<void> {
        template<typename E>
        friend class ec;

      public:
        using result_type = void;
        using completion_type = completion<result_type>;

        iop(completion_type *c) : comp{c} {}
        ~iop();

        iop(iop const &) = delete;
        iop &operator=(iop const &) = delete;
        iop(iop &&i) : comp{std::exchange(i.comp, {})} {}
        iop &operator=(iop &&i) {
            std::swap(i.comp, comp);
            return *this;
        }

        bool await_ready() const noexcept { return false; }
        felspar::coro::coroutine_handle<>
                await_suspend(felspar::coro::coroutine_handle<> h) {
            return comp->await_suspend(h);
        }
        auto await_resume() {
            if (comp->result.error) { comp->result.throw_exception(comp->loc); }
        }

      private:
        completion_type *comp;
    };


}
