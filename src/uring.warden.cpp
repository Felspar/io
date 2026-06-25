#include "uring.hpp"

#include <felspar/exceptions/runtime_error.hpp>


/// ## `felspar::io::uring_warden`


felspar::io::uring_warden::uring_warden(unsigned entries, unsigned flags)
: ring{std::make_unique<impl>()} {
    if (auto const ret = ::io_uring_queue_init(entries, &ring->uring, flags);
        ret < 0) {
        throw felspar::stdexcept::system_error{
                -ret, std::system_category(), "uring_queue_init"};
    }
}
felspar::io::uring_warden::~uring_warden() {
    if (ring) { ::io_uring_queue_exit(&ring->uring); }
}


void felspar::io::uring_warden::run_until(std::coroutine_handle<> coro) {
    coro.resume();
    while (not coro.done()) {
        ::io_uring_submit(&ring->uring);

        ::io_uring_cqe *cqe = {};
        auto const ret = ::io_uring_wait_cqe(&ring->uring, &cqe);
        if (ret < 0) {
            throw felspar::stdexcept::system_error{
                    -ret, std::system_category(), "uring_wait_cqe"};
        }
        ring->execute(cqe);
        while (::io_uring_peek_cqe(&ring->uring, &cqe) == 0) {
            ring->execute(cqe);
        }
    }
}


void felspar::io::uring_warden::run_batch() {
    ::io_uring_submit(&ring->uring);
    ::io_uring_cqe *cqe = {};
    while (::io_uring_peek_cqe(&ring->uring, &cqe) == 0) { ring->execute(cqe); }
}


/// ## `felspar::io::uring_warden::impl`


::io_uring_sqe *felspar::io::uring_warden::impl::next_sqe() {
    ::io_uring_sqe *sqe = ::io_uring_get_sqe(&uring);
    if (not sqe) {
        throw felspar::stdexcept::runtime_error{
                "No more SQEs are available in the ring"};
    }
    return sqe;
}


void felspar::io::uring_warden::impl::execute(::io_uring_cqe *cqe) {
    auto d = reinterpret_cast<delivery *>(::io_uring_cqe_get_data(cqe));
    int result = cqe->res;
    ::io_uring_cqe_seen(&uring, cqe);
    /**
     * If the owning IOP has already been destructed (for example its coroutine
     * was cancelled while this op was still in flight) the completion is only
     * being kept alive to drain its outstanding CQEs. There is no coroutine
     * left to resume, so calling `deliver` would resume a destroyed handle and
     * read freed memory -- we just count the CQE down and let the bookkeeping
     * below delete the completion once the last one arrives.
     */
    if (d->iop_exists) { d->deliver(result); }
    if (d->is_outstanding) { std::erase(outstanding, d); }
    if (--d->iop_count == 0 and not d->iop_exists) { delete d; }
}
