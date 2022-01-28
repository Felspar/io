#pragma once


#include <felspar/exceptions.hpp>
#include <felspar/io/warden.io_uring.hpp>

#include <liburing.h>


namespace felspar::io {


    struct io_uring_warden::impl {
        ::io_uring uring;

        /// Fetch another SQE from the io_uring
        io_uring_sqe *next_sqe();

        void execute(::io_uring_cqe *);
    };


    struct io_uring_warden::delivery {
        virtual void deliver(int result) = 0;
    };
    template<typename R>
    struct io_uring_warden::completion :
    public delivery,
            public io::completion<R> {
        completion(io_uring_warden *w, felspar::source_location loc)
        : io::completion<R>{std::move(loc)}, self{w} {}

        io_uring_warden *self;

        warden *ward() override { return self; }
        io_uring_sqe *setup_submission(felspar::coro::coroutine_handle<> h) {
            io::completion<R>::handle = h;
            auto sqe = self->ring->next_sqe();
            ::io_uring_sqe_set_data(sqe, this);
            return sqe;
        }
        void deliver(int result) override {
            if (result < 0) {
                io::completion<R>::exception = std::make_exception_ptr(
                        felspar::stdexcept::system_error{
                                -result, std::generic_category(),
                                "io_uring IOP",
                                std::move(io::completion<R>::loc)});
                io::completion<R>::handle.resume();
            } else {
                io::completion<R>::result = result;
                io::completion<R>::handle.resume();
            }
        }
    };
    template<>
    struct io_uring_warden::completion<void> :
    public delivery,
            public io::completion<void> {
        completion(io_uring_warden *w, felspar::source_location loc)
        : io::completion<void>{std::move(loc)}, self{w} {}

        io_uring_warden *self;

        warden *ward() override { return self; }
        io_uring_sqe *setup_submission(felspar::coro::coroutine_handle<> h) {
            io::completion<void>::handle = h;
            auto sqe = self->ring->next_sqe();
            return sqe;
        }
        void deliver(int result) override {
            if (result < 0) {
                io::completion<void>::exception = std::make_exception_ptr(
                        felspar::stdexcept::system_error{
                                -result, std::generic_category(),
                                "io_uring IOP",
                                std::move(io::completion<void>::loc)});
            } else {
                io::completion<void>::handle.resume();
            }
        }
    };


}
