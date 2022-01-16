#include "io_uring.hpp"

#include <felspar/exceptions.hpp>


/**
 * `felspar::poll::io_uring_warden`
 */


felspar::poll::io_uring_warden::io_uring_warden(unsigned entries, unsigned flags)
: ring{std::make_unique<impl>()} {
    if (auto const ret =
                ::io_uring_queue_init(entries, &ring->uring, flags) < 0) {
        throw felspar::stdexcept::system_error{
                -ret, std::generic_category(), "io_uring_queue_init"};
    }
}
felspar::poll::io_uring_warden::~io_uring_warden() {
    if (ring) { ::io_uring_queue_exit(&ring->uring); }
}


void felspar::poll::io_uring_warden::run_until(
        felspar::coro::unique_handle<felspar::coro::task_promise<void>> coro) {
    coro.resume();
    while (not coro.done()) {
        ::io_uring_submit(&ring->uring);

        ::io_uring_cqe *cqe;
        int ret = io_uring_wait_cqe(&ring->uring, &cqe);
        completion::execute(&ring->uring, cqe);
        while (::io_uring_peek_cqe(&ring->uring, &cqe) == 0) {
            completion::execute(&ring->uring, cqe);
        }
    }
    coro.promise().consume_value();
}


void felspar::poll::io_uring_warden::cancel(poll::completion *p) { delete p; }


/**
 * `felspar::poll::io_uring_warden::completion`
 */


void felspar::poll::io_uring_warden::completion::execute(
        ::io_uring *uring, ::io_uring_cqe *cqe) {
    auto comp = reinterpret_cast<felspar::poll::io_uring_warden::completion *>(
            io_uring_cqe_get_data(cqe));
    comp->result = cqe->res;
    ::io_uring_cqe_seen(uring, cqe);
    if (comp->handle) {
        comp->handle.resume();
    } else {
        throw felspar::stdexcept::runtime_error{"No coroutine handle"};
    }
}


/**
 * `felspar::poll::io_uring_warden::impl`
 */


io_uring_sqe *felspar::poll::io_uring_warden::impl::next_sqe() {
    io_uring_sqe *sqe = io_uring_get_sqe(&uring);
    if (not sqe) {
        throw felspar::stdexcept::runtime_error{
                "No more SQEs are available in the ring"};
    }
    return sqe;
}
