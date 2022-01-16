#pragma once


#include <felspar/poll/warden.io_uring.hpp>

#include <liburing.h>


namespace felspar::poll {


    struct io_uring_warden::completion : public poll::completion {
        completion(io_uring_warden *w) : self{w} {}

        warden *ward() override { return self; }
        void await_suspend(felspar::coro::coroutine_handle<> h) override {
            handle = h;
        }

        io_uring_warden *self;
        sockaddr addr = {};
        socklen_t addrlen = {};

        static void execute(::io_uring *, ::io_uring_cqe *);
    };


    struct io_uring_warden::impl {
        ::io_uring uring;

        /// Fetch another SQE from the io_uring
        io_uring_sqe *next_sqe();
    };


}
