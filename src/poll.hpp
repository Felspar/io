#pragma once


#include <felspar/io/warden.poll.hpp>


namespace felspar::io {


    struct poll_warden::retrier {
        virtual void try_or_resume() = 0;
    };


    template<typename R>
    struct poll_warden::completion : public retrier, public io::completion<R> {
        completion(poll_warden *w, felspar::source_location loc)
        : io::completion<R>{std::move(loc)}, self{w} {}

        poll_warden *self;
        warden *ward() override { return self; }

        void await_suspend(felspar::coro::coroutine_handle<> h) override {
            io::completion<R>::handle = h;
            try_or_resume();
        }
    };


}
