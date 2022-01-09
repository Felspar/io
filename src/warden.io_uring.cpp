#include <felspar/poll/warden.io_uring.hpp>


felspar::poll::io_uring_warden::io_uring_warden() {}


void felspar::poll::io_uring_warden::run_until(
        felspar::coro::unique_handle<felspar::coro::task_promise<void>>) {}


void felspar::poll::io_uring_warden::cancel(iop<void>::completion *p) {
    delete p;
}


felspar::poll::iop<int> felspar::poll::io_uring_warden::accept(int fd) {
    return {nullptr};
}


felspar::poll::iop<void> felspar::poll::io_uring_warden::connect(
        int fd, sockaddr const *, socklen_t) {
    return {nullptr};
}


felspar::poll::iop<void> felspar::poll::io_uring_warden::read_ready(int fd) {
    return {nullptr};
}


felspar::poll::iop<void> felspar::poll::io_uring_warden::write_ready(int fd) {
    return {nullptr};
}
