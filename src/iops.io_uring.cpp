#include "io_uring.hpp"

#include <felspar/exceptions.hpp>

#include <poll.h>


felspar::poll::iop<std::size_t> felspar::poll::io_uring_warden::read_some(
        int fd, std::span<std::byte> b, felspar::source_location loc) {
    auto *c = new completion{this, [](completion *c) {
                                 auto sqe = c->self->ring->next_sqe();
                                 ::io_uring_prep_read(
                                         sqe, c->fd, c->bytes.data(),
                                         c->bytes.size(), 0);
                                 ::io_uring_sqe_set_data(sqe, c);
                             }};
    c->fd = fd;
    c->bytes = b;
    return {c};
}
felspar::poll::iop<std::size_t> felspar::poll::io_uring_warden::write_some(
        int fd, std::span<std::byte const> b, felspar::source_location loc) {
    auto *c = new completion{this, [](completion *c) {
                                 auto sqe = c->self->ring->next_sqe();
                                 ::io_uring_prep_write(
                                         sqe, c->fd, c->cbytes.data(),
                                         c->cbytes.size(), 0);
                                 ::io_uring_sqe_set_data(sqe, c);
                             }};
    c->fd = fd;
    c->cbytes = b;
    return {c};
}


felspar::poll::iop<int> felspar::poll::io_uring_warden::accept(
        int fd, felspar::source_location loc) {
    auto *c = new completion{this, [](completion *c) {
                                 auto sqe = c->self->ring->next_sqe();
                                 ::io_uring_prep_accept(
                                         sqe, c->fd, &c->addr, &c->addrlen, 0);
                                 ::io_uring_sqe_set_data(sqe, c);
                             }};
    c->fd = fd;
    return {c};
}


felspar::poll::iop<void> felspar::poll::io_uring_warden::connect(
        int fd,
        sockaddr const *addr,
        socklen_t addrlen,
        felspar::source_location loc) {
    auto *c = new completion{this, [](completion *c) {
                                 auto sqe = c->self->ring->next_sqe();
                                 ::io_uring_prep_connect(
                                         sqe, c->fd, c->addrptr, c->addrlen);
                                 ::io_uring_sqe_set_data(sqe, c);
                             }};
    c->fd = fd;
    c->addrptr = addr;
    c->addrlen = addrlen;
    return {c};
}


felspar::poll::iop<void> felspar::poll::io_uring_warden::read_ready(
        int fd, felspar::source_location loc) {
    auto *c = new completion{this, [](completion *c) {
                                 auto sqe = c->self->ring->next_sqe();
                                 ::io_uring_prep_poll_add(sqe, c->fd, POLLIN);
                                 ::io_uring_sqe_set_data(sqe, c);
                             }};
    c->fd = fd;
    return {c};
}


felspar::poll::iop<void> felspar::poll::io_uring_warden::write_ready(
        int fd, felspar::source_location loc) {
    auto *c = new completion{this, [](completion *c) {
                                 auto sqe = c->self->ring->next_sqe();
                                 ::io_uring_prep_poll_add(sqe, c->fd, POLLOUT);
                                 ::io_uring_sqe_set_data(sqe, c);
                             }};
    c->fd = fd;
    return {c};
}
