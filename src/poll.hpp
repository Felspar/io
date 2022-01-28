#pragma once


#include <felspar/poll/warden.poll.hpp>


namespace felspar::poll {


    struct poll_warden::retrier {
        virtual void try_or_resume() = 0;
    };


    template<typename R>
    struct poll_warden::completion :
    public retrier,
            public poll::completion<R> {
        completion(poll_warden *w, felspar::source_location loc)
        : poll::completion<R>{std::move(loc)}, self{w} {}

        poll_warden *self;
        warden *ward() override { return self; }

        void await_suspend(felspar::coro::coroutine_handle<> h) override {
            poll::completion<R>::handle = h;
            try_or_resume();
        }
    };


}
