#pragma once


#include <felspar/io/completion.hpp>


namespace felspar::io {


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
        auto await_resume() { return std::move(wrapped.comp->result); }
    };


}
