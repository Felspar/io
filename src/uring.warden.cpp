#include "uring.hpp"


/**
 * `felspar::io::uring_warden`
 */


felspar::io::uring_warden::uring_warden(unsigned entries, unsigned flags)
: ring{std::make_unique<impl>()} {
    if (auto const ret =
                ::io_uring_queue_init(entries, &ring->uring, flags) < 0) {
        throw felspar::stdexcept::system_error{
                -ret, std::system_category(), "uring_queue_init"};
    }
}
felspar::io::uring_warden::~uring_warden() {
    if (ring) { ::io_uring_queue_exit(&ring->uring); }
}


void felspar::io::uring_warden::run_until(
        felspar::coro::coroutine_handle<> coro) {
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


/**
 * `felspar::io::uring_warden::impl`
 */


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
    d->deliver(result);
    if (d->is_outstanding) { std::erase(outstanding, d); }
    if (--d->iop_count == 0 and not d->iop_exists) { delete d; }
}
