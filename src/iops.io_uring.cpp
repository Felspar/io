#include "io_uring.hpp"

#include <felspar/exceptions.hpp>

#include <poll.h>


felspar::poll::iop<int> felspar::poll::io_uring_warden::accept(
        int fd, felspar::source_location loc) {
    auto sqe = ring->next_sqe();
    auto *c = new completion{this};
    ::io_uring_prep_accept(sqe, fd, &c->addr, &c->addrlen, 0);
    ::io_uring_sqe_set_data(sqe, c);
    return {c};
}


felspar::poll::iop<void> felspar::poll::io_uring_warden::connect(
        int fd,
        sockaddr const *addr,
        socklen_t addrlen,
        felspar::source_location loc) {
    auto sqe = ring->next_sqe();
    auto *c = new completion{this};
    ::io_uring_prep_connect(sqe, fd, addr, addrlen);
    ::io_uring_sqe_set_data(sqe, c);
    return {c};
}


felspar::poll::iop<void> felspar::poll::io_uring_warden::read_ready(
        int fd, felspar::source_location loc) {
    auto sqe = ring->next_sqe();
    auto *c = new completion{this};
    ::io_uring_prep_poll_add(sqe, fd, POLLIN);
    ::io_uring_sqe_set_data(sqe, c);
    return {c};
}


felspar::poll::iop<void> felspar::poll::io_uring_warden::write_ready(
        int fd, felspar::source_location loc) {
    auto sqe = ring->next_sqe();
    auto *c = new completion{this};
    ::io_uring_prep_poll_add(sqe, fd, POLLOUT);
    ::io_uring_sqe_set_data(sqe, c);
    return {c};
}
