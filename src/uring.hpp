#pragma once


#include <felspar/exceptions.hpp>
#include <felspar/io/warden.uring.hpp>

#include <liburing.h>

#include <vector>


namespace felspar::io {


    struct uring_warden::delivery {
        /// True so long as the IOP instance still exists
        bool iop_exists = true;
        /// True if the completion is now in the outstanding completions list
        bool is_outstanding = false;
        /// The number of IOPs that have been submitted to the queue and not
        /// returned
        std::size_t iop_count = 1;

        virtual ~delivery() = default;
        virtual void deliver(int result) = 0;
    };


    struct uring_warden::impl {
        ~impl() {
            for (auto *d : outstanding) { delete d; }
        }

        ::io_uring uring;

        /// Fetch another SQE from the uring
        ::io_uring_sqe *next_sqe();

        void execute(::io_uring_cqe *);

        std::vector<delivery *> outstanding;
    };


    template<typename R>
    struct uring_warden::completion :
    public delivery,
            public io::completion<R> {
        completion(
                uring_warden *w,
                std::optional<std::chrono::nanoseconds> tout,
                felspar::source_location const &loc)
        : io::completion<R>{loc}, self{w}, timeout{tout} {}

        uring_warden *self = nullptr;
        warden *ward() override { return self; }

        std::optional<std::chrono::nanoseconds> timeout = {};
        __kernel_timespec kts;

        ::io_uring_sqe *setup_submission(std::coroutine_handle<> h) {
            io::completion<R>::handle = h;
            return self->ring->next_sqe();
        }
        std::coroutine_handle<> setup_timeout(::io_uring_sqe *sqe) {
            ::io_uring_sqe_set_data(sqe, this);
            if (timeout) {
                sqe->flags |= IOSQE_IO_LINK;
                auto tsqe = self->ring->next_sqe();
                kts.tv_sec = 0;
                kts.tv_nsec = timeout->count();
                ::io_uring_prep_link_timeout(tsqe, &kts, 0);
                ::io_uring_sqe_set_data(tsqe, this);
                iop_count = 2;
            }
            return std::noop_coroutine();
        }

        void deliver(int result) override {
            if (result < 0) {
                if (timeout and result == -ECANCELED) {
                    /// This is the cancelled IOP so we ignore it as we've timed
                    /// out
                    return;
                } else {
                    io::completion<R>::result = {
                            {-result, std::system_category()}, "uring IOP"};
                }
            } else {
                io::completion<R>::result = result;
            }
            io::completion<R>::handle.resume();
        }
        bool delete_due_to_iop_destructed() override {
            iop_exists = false;
            if (iop_count == 0) {
                return true;
            } else {
                /// Cancel IOP
                is_outstanding = true;
                self->ring->outstanding.push_back(this);
                return false;
            }
        }
    };
    template<>
    struct uring_warden::completion<void> :
    public delivery,
            public io::completion<void> {
        completion(
                uring_warden *w,
                std::optional<std::chrono::nanoseconds> tout,
                felspar::source_location const &loc)
        : io::completion<void>{loc}, self{w}, timeout{tout} {}

        uring_warden *self;
        warden *ward() override { return self; }

        std::optional<std::chrono::nanoseconds> timeout = {};
        __kernel_timespec kts;

        ::io_uring_sqe *setup_submission(std::coroutine_handle<> h) {
            io::completion<void>::handle = h;
            return self->ring->next_sqe();
        }
        std::coroutine_handle<> setup_timeout(::io_uring_sqe *sqe) {
            ::io_uring_sqe_set_data(sqe, this);
            if (timeout) {
                sqe->flags |= IOSQE_IO_LINK;
                auto tsqe = self->ring->next_sqe();
                kts.tv_sec = 0;
                kts.tv_nsec = timeout->count();
                ::io_uring_prep_link_timeout(tsqe, &kts, 0);
                ::io_uring_sqe_set_data(tsqe, this);
                iop_count = 2;
            }
            return std::noop_coroutine();
        }

        void deliver(int result) override {
            if (result == -ETIME) {
                io::completion<void>::result = {
                        {ETIME, std::system_category()}, "uring IOP timeout"};
            } else if (timeout and result == -ECANCELED) {
                /// This is the cancelled IOP so we ignore it as we've timed
                /// out
                return;
            } else if (result < 0) {
                io::completion<void>::result = {
                        {-result, std::system_category()}, "uring IOP"};
            }
            io::completion<void>::handle.resume();
        }
        bool delete_due_to_iop_destructed() override {
            iop_exists = false;
            if (iop_count == 0) {
                return true;
            } else {
                /// Cancel IOP
                is_outstanding = true;
                self->ring->outstanding.push_back(this);
                return false;
            }
        }
    };


}
