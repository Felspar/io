#pragma once


#include <felspar/exceptions.hpp>
#include <felspar/io/exceptions.hpp>
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
        completion(
                uring_warden *w,
                std::optional<std::chrono::nanoseconds> tout,
                felspar::source_location loc)
        : io::completion<R>{std::move(loc)}, self{w}, timeout{std::move(tout)} {}
        completion(uring_warden *w, felspar::source_location loc)
        : io::completion<R>{std::move(loc)}, self{w} {}

        uring_warden *self = nullptr;
        warden *ward() override { return self; }

        std::optional<std::chrono::nanoseconds> timeout = {};
        __kernel_timespec kts;

        ::io_uring_sqe *setup_submission(felspar::coro::coroutine_handle<> h) {
            io::completion<R>::handle = h;
            return self->ring->next_sqe();
        }
        felspar::coro::coroutine_handle<> setup_timeout(::io_uring_sqe *sqe) {
            ::io_uring_sqe_set_data(sqe, this);
            if (timeout) {
                sqe->flags |= IOSQE_IO_LINK;
                auto tsqe = self->ring->next_sqe();
                kts.tv_sec = 0;
                kts.tv_nsec = timeout->count();
                ::io_uring_prep_link_timeout(tsqe, &kts, 0);
                ::io_uring_sqe_set_data(tsqe, this);
                io::completion<R>::iop_count = 2;
            }
            return felspar::coro::noop_coroutine();
        }

        void deliver(int result) override {
            if (result < 0) {
                if (result == -ETIME) {
                    io::completion<R>::exception =
                            std::make_exception_ptr(io::timeout{
                                    "uring IOP timeout",
                                    std::move(io::completion<R>::loc)});
                } else if (timeout and result == -ECANCELED) {
                    /// This is the cancelled IOP so we ignore it as we've timed
                    /// out
                    self->cancel(this);
                    return;
                } else {
                    io::completion<R>::exception =
                            std::make_exception_ptr(stdexcept::system_error{
                                    -result, std::generic_category(),
                                    "uring IOP",
                                    std::move(io::completion<R>::loc)});
                }
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
        completion(
                uring_warden *w,
                std::optional<std::chrono::nanoseconds> tout,
                felspar::source_location loc)
        : io::completion<void>{std::move(loc)},
          self{w},
          timeout{std::move(tout)} {}
        completion(uring_warden *w, felspar::source_location loc)
        : io::completion<void>{std::move(loc)}, self{w} {}

        uring_warden *self;
        warden *ward() override { return self; }

        std::optional<std::chrono::nanoseconds> timeout = {};
        __kernel_timespec kts;

        ::io_uring_sqe *setup_submission(felspar::coro::coroutine_handle<> h) {
            io::completion<void>::handle = h;
            return self->ring->next_sqe();
        }
        felspar::coro::coroutine_handle<> setup_timeout(::io_uring_sqe *sqe) {
            ::io_uring_sqe_set_data(sqe, this);
            if (timeout) {
                sqe->flags |= IOSQE_IO_LINK;
                auto tsqe = self->ring->next_sqe();
                kts.tv_sec = 0;
                kts.tv_nsec = timeout->count();
                ::io_uring_prep_link_timeout(tsqe, &kts, 0);
                ::io_uring_sqe_set_data(tsqe, this);
                io::completion<void>::iop_count = 2;
            }
            return felspar::coro::noop_coroutine();
        }

        void deliver(int result) override {
            if (result == -ETIME) {
                io::completion<void>::exception =
                        std::make_exception_ptr(io::timeout{
                                "uring IOP timeout",
                                std::move(io::completion<void>::loc)});
            } else if (timeout and result == -ECANCELED) {
                /// This is the cancelled IOP so we ignore it as we've timed
                /// out
                self->cancel(this);
                return;
            } else if (result < 0) {
                io::completion<void>::exception = std::make_exception_ptr(
                        felspar::stdexcept::system_error{
                                -result, std::generic_category(), "uring IOP",
                                std::move(io::completion<void>::loc)});
            }
            io::completion<void>::handle.resume();
        }
    };


}
