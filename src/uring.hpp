#pragma once


#include <felspar/exceptions.hpp>
#include <felspar/io/warden.uring.hpp>

#include <liburing.h>


namespace felspar::io {


    struct uring_warden::impl {
        ::io_uring uring;

        /// Fetch another SQE from the uring
        ::io_uring_sqe *next_sqe();

        void execute(::io_uring_cqe *);
    };


    struct uring_warden::delivery {
        virtual void deliver(int result) = 0;
    };
    template<typename R>
    struct uring_warden::completion :
    public delivery,
            public io::completion<R> {
        completion(uring_warden *w, felspar::source_location loc)
        : io::completion<R>{std::move(loc)}, self{w} {}

        uring_warden *self;
        warden *ward() override { return self; }

        ::io_uring_sqe *setup_submission(felspar::coro::coroutine_handle<> h) {
            io::completion<R>::handle = h;
            return self->ring->next_sqe();
        }

        void deliver(int result) override {
            if (result < 0) {
                io::completion<R>::exception = std::make_exception_ptr(
                        felspar::stdexcept::system_error{
                                -result, std::generic_category(), "uring IOP",
                                std::move(io::completion<R>::loc)});
            } else {
                io::completion<R>::result = result;
            }
            io::completion<R>::handle.resume();
        }
    };
    template<>
    struct uring_warden::completion<void> :
    public delivery,
            public io::completion<void> {
        completion(uring_warden *w, felspar::source_location loc)
        : io::completion<void>{std::move(loc)}, self{w} {}

        uring_warden *self;
        warden *ward() override { return self; }

        ::io_uring_sqe *setup_submission(felspar::coro::coroutine_handle<> h) {
            io::completion<void>::handle = h;
            return self->ring->next_sqe();
        }

        void deliver(int result) override {
            if (result < 0) {
                io::completion<void>::exception = std::make_exception_ptr(
                        felspar::stdexcept::system_error{
                                -result, std::generic_category(), "uring IOP",
                                std::move(io::completion<void>::loc)});
            }
            io::completion<void>::handle.resume();
        }
    };


}
