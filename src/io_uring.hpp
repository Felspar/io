#pragma once


#include <felspar/poll/warden.io_uring.hpp>

#include <liburing.h>


namespace felspar::poll {


    struct io_uring_warden::impl {
        ::io_uring uring;

        /// Fetch another SQE from the io_uring
        io_uring_sqe *next_sqe();
    };


    struct io_uring_warden::completion : public poll::completion {
        completion(io_uring_warden *w, void (*o)(completion *))
        : self{w}, order_iop{o} {}

        io_uring_warden *self;
        void (*order_iop)(completion *);

        warden *ward() override { return self; }
        void await_suspend(felspar::coro::coroutine_handle<> h) override {
            handle = h;
            order_iop(this);
        }

        /// Extra storage that can be used for "stuff" important to IOPs
        int fd = {};
        sockaddr addr = {};
        sockaddr const *addrptr = {};
        socklen_t addrlen = {};
        std::span<std::byte> bytes = {};
        std::span<std::byte const> cbytes = {};

        static void execute(::io_uring *, ::io_uring_cqe *);
    };


}
