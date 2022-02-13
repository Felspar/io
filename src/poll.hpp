#pragma once


#include <felspar/io/exceptions.hpp>
#include <felspar/io/warden.poll.hpp>


namespace felspar::io {


    struct poll_warden::retrier {
        virtual felspar::coro::coroutine_handle<> try_or_resume() = 0;
        virtual void iop_timedout() = 0;
    };


    template<typename R>
    struct poll_warden::completion : public retrier, public io::completion<R> {
        completion(
                poll_warden *w,
                std::optional<std::chrono::nanoseconds> t,
                felspar::source_location loc)
        : io::completion<R>{std::move(loc)}, self{w}, timeout{std::move(t)} {}
        completion(poll_warden *w, felspar::source_location loc)
        : io::completion<R>{std::move(loc)}, self{w} {}

        poll_warden *self;
        warden *ward() override { return self; }

        std::optional<std::chrono::nanoseconds> timeout = {};

        felspar::coro::coroutine_handle<>
                await_suspend(felspar::coro::coroutine_handle<> h) override {
            io::completion<R>::handle = h;
            if (timeout) {
                auto const deadline =
                        std::chrono::steady_clock::now() + *timeout;
                self->timeouts.insert({deadline, this});
            }
            return try_or_resume();
        }
        void iop_timedout() override {
            io::completion<R>::exception = std::make_exception_ptr(io::timeout{
                    "poll IOP timeout", std::move(io::completion<R>::loc)});
            io::completion<R>::handle.resume();
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
