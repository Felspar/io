#pragma once


#include <felspar/io/exceptions.hpp>
#include <felspar/io/warden.poll.hpp>


namespace felspar::io {


    struct poll_warden::retrier {
        virtual std::coroutine_handle<> try_or_resume() = 0;
        virtual std::coroutine_handle<> iop_timedout() = 0;
    };


    template<typename R>
    struct poll_warden::completion : public retrier, public io::completion<R> {
        completion(
                poll_warden *w,
                std::optional<std::chrono::nanoseconds> t,
                std::source_location const &loc)
        : io::completion<R>{loc}, self{w}, timeout{t} {}

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
        void cancel_timeout() {
            if (timeout) {
                auto pos = std::find_if(
                        self->timeouts.begin(), self->timeouts.end(),
                        [this](auto const &s) { return s.second == this; });
                if (pos != self->timeouts.end()) { self->timeouts.erase(pos); }
            }
        }
        virtual void cancel_iop() = 0;

        std::coroutine_handle<>
                await_suspend(std::coroutine_handle<> h) override {
            io::completion<R>::handle = h;
            insert_timeout();
            return try_or_resume();
        }
        std::coroutine_handle<> iop_timedout() override {
            cancel_iop();
            io::completion<R>::result = {timeout::error, "IOP timed out"};
            return io::completion<R>::handle;
        }
        std::coroutine_handle<> cancel_timeout_then_resume() {
            cancel_timeout();
            return io::completion<R>::handle;
        }

        bool delete_due_to_iop_destructed() override {
            cancel_timeout();
            cancel_iop();
            return true;
        }
    };

    /// These are used to smooth over some Windows/POSIX differences
    inline bool would_block(auto const err) {
#ifdef FELSPAR_WINSOCK2
        return err == WSAEWOULDBLOCK;
#else
        return err == EAGAIN or err == EWOULDBLOCK;
#endif
    }
    inline bool bad_fd(auto const err) {
#ifdef FELSPAR_WINSOCK2
        return false;
#else
        return err == EBADF;
#endif
    }


}
