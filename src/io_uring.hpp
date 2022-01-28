#pragma once


#include <felspar/exceptions.hpp>
#include <felspar/poll/warden.io_uring.hpp>

#include <liburing.h>


namespace felspar::poll {


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
            public poll::completion<R> {
        completion(io_uring_warden *w, felspar::source_location loc)
        : poll::completion<R>{std::move(loc)}, self{w} {}

        io_uring_warden *self;

        warden *ward() override { return self; }
        void deliver(int result) override {
            if (result < 0) {
                poll::completion<R>::exception = std::make_exception_ptr(
                        felspar::stdexcept::system_error{
                                -result, std::generic_category(),
                                "io_uring IOP"});
                poll::completion<R>::handle.resume();
            } else {
                poll::completion<R>::result = result;
                poll::completion<R>::handle.resume();
            }
        }
    };
    template<>
    struct io_uring_warden::completion<void> :
    public delivery,
            public poll::completion<void> {
        completion(io_uring_warden *w, felspar::source_location loc)
        : poll::completion<void>{std::move(loc)}, self{w} {}

        io_uring_warden *self;

        warden *ward() override { return self; }
        void deliver(int result) override {
            if (result < 0) {
                poll::completion<void>::exception = std::make_exception_ptr(
                        felspar::stdexcept::system_error{
                                -result, std::generic_category(),
                                "io_uring IOP"});
            } else {
                poll::completion<void>::handle.resume();
            }
        }
    };


}
