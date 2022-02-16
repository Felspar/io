#pragma once


#include <felspar/io/exceptions.hpp>
#include <felspar/io/warden.poll.hpp>


namespace felspar::io {


    struct poll_warden::retrier {
        virtual felspar::coro::coroutine_handle<> try_or_resume() = 0;
        virtual felspar::coro::coroutine_handle<> iop_timedout() = 0;
    };


    template<typename R>
    struct poll_warden::completion : public retrier, public io::completion<R> {
        completion(
                poll_warden *w,
                std::optional<std::chrono::nanoseconds> t,
                felspar::source_location loc)
        : io::completion<R>{std::move(loc)}, self{w}, timeout{std::move(t)} {}

        poll_warden *self;
        warden *ward() override { return self; }

        std::optional<std::chrono::nanoseconds> timeout = {};

        void insert_timeout() {
            if (timeout) {
                auto const deadline =
                        std::chrono::steady_clock::now() + *timeout;
                self->timeouts.insert({deadline, this});
            }
        }
        felspar::coro::coroutine_handle<>
                await_suspend(felspar::coro::coroutine_handle<> h) override {
            io::completion<R>::handle = h;
            insert_timeout();
            return try_or_resume();
        }
        felspar::coro::coroutine_handle<> iop_timedout() override {
            io::completion<R>::error.code = timeout::error;
            io::completion<R>::error.message = "IOP timed out";
            return io::completion<R>::handle;
        }
        felspar::coro::coroutine_handle<> cancel_timeout_then_resume() {
            if (timeout) {
                auto pos = std::find_if(
                        self->timeouts.begin(), self->timeouts.end(),
                        [this](auto const &s) { return s.second == this; });
                if (pos != self->timeouts.end()) { self->timeouts.erase(pos); }
            }
            return io::completion<R>::handle;
        }
    };


}
