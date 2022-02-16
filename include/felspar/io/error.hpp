#pragma once


#include <felspar/io/completion.hpp>


namespace felspar::io {


    /// The result of an IOP for the case where we don't throw exceptions for
    /// errors
    template<typename R>
    struct result {
        result(completion<R> *c) : value{}, error{c->error.code} {
            if (not error) { value = std::move(c->result); }
        }
        std::optional<R> value;
        std::error_code error;

        explicit operator bool() const noexcept {
            return not static_cast<bool>(error);
        }
    };
    template<>
    struct result<void> {
        result(completion<void> *c) : error{c->error.code} {}
        std::error_code error;

        explicit operator bool() const noexcept {
            return not static_cast<bool>(error);
        }
    };


    /// Wrapper around an IOP to directly expose errors instead of throwing
    template<typename R>
    class ec {
        iop<R> wrapped;

      public:
        ec(iop<R> &&i) : wrapped{std::move(i)} {}

        bool await_ready() const noexcept { return wrapped.await_ready(); }
        felspar::coro::coroutine_handle<>
                await_suspend(felspar::coro::coroutine_handle<> h) {
            return wrapped.await_suspend(h);
        }
        auto await_resume() { return result<R>{wrapped.comp}; }
    };


}
