#include "io_uring.hpp"

#include <poll.h>


felspar::poll::iop<std::size_t> felspar::poll::io_uring_warden::read_some(
        int fd, std::span<std::byte> b, felspar::source_location loc) {
    struct c : public completion<std::size_t> {
        c(io_uring_warden *s,
          int f,
          std::span<std::byte> b,
          felspar::source_location loc)
        : completion<std::size_t>{s, std::move(loc)}, fd{f}, bytes{b} {}
        int fd;
        std::span<std::byte> bytes;
        void await_suspend(felspar::coro::coroutine_handle<> h) override {
            handle = h;
            auto sqe = self->ring->next_sqe();
            ::io_uring_prep_read(sqe, fd, bytes.data(), bytes.size(), 0);
            ::io_uring_sqe_set_data(sqe, this);
        }
    };
    return {new c{this, fd, b, std::move(loc)}};
}
felspar::poll::iop<std::size_t> felspar::poll::io_uring_warden::write_some(
        int fd, std::span<std::byte const> b, felspar::source_location loc) {
    struct c : public completion<std::size_t> {
        c(io_uring_warden *s,
          int f,
          std::span<std::byte const> b,
          felspar::source_location loc)
        : completion<std::size_t>{s, std::move(loc)}, fd{f}, bytes{b} {}
        int fd;
        std::span<std::byte const> bytes;
        void await_suspend(felspar::coro::coroutine_handle<> h) override {
            handle = h;
            auto sqe = self->ring->next_sqe();
            ::io_uring_prep_write(sqe, fd, bytes.data(), bytes.size(), 0);
            ::io_uring_sqe_set_data(sqe, this);
        }
    };
    return {new c{this, fd, b, std::move(loc)}};
}


felspar::poll::iop<int> felspar::poll::io_uring_warden::accept(
        int fd, felspar::source_location loc) {
    struct c : public completion<int> {
        c(io_uring_warden *s, int f, felspar::source_location loc)
        : completion<int>{s, std::move(loc)}, fd{f} {}
        int fd = {};
        sockaddr addr = {};
        socklen_t addrlen = {};
        void await_suspend(felspar::coro::coroutine_handle<> h) override {
            handle = h;
            auto sqe = self->ring->next_sqe();
            ::io_uring_prep_accept(sqe, fd, &addr, &addrlen, 0);
            ::io_uring_sqe_set_data(sqe, this);
        }
    };
    return {new c{this, fd, std::move(loc)}};
}


felspar::poll::iop<void> felspar::poll::io_uring_warden::connect(
        int fd,
        sockaddr const *addr,
        socklen_t addrlen,
        felspar::source_location loc) {
    struct c : public completion<void> {
        c(io_uring_warden *s,
          int f,
          sockaddr const *a,
          socklen_t l,
          felspar::source_location loc)
        : completion<void>{s, std::move(loc)}, fd{f}, addr{a}, addrlen{l} {}
        int fd = {};
        sockaddr const *addr = {};
        socklen_t addrlen = {};
        void await_suspend(felspar::coro::coroutine_handle<> h) override {
            handle = h;
            auto sqe = self->ring->next_sqe();
            ::io_uring_prep_connect(sqe, fd, addr, addrlen);
            ::io_uring_sqe_set_data(sqe, this);
        }
    };
    return {new c{this, fd, addr, addrlen, std::move(loc)}};
}


felspar::poll::iop<void> felspar::poll::io_uring_warden::read_ready(
        int fd, felspar::source_location loc) {
    struct c : public completion<void> {
        c(io_uring_warden *s, int f, felspar::source_location loc)
        : completion<void>{s, std::move(loc)}, fd{f} {}
        int fd = {};
        void await_suspend(felspar::coro::coroutine_handle<> h) override {
            handle = h;
            auto sqe = self->ring->next_sqe();
            ::io_uring_prep_poll_add(sqe, fd, POLLIN);
            ::io_uring_sqe_set_data(sqe, this);
        }
    };
    return {new c{this, fd, std::move(loc)}};
}


felspar::poll::iop<void> felspar::poll::io_uring_warden::write_ready(
        int fd, felspar::source_location loc) {
    struct c : public completion<void> {
        c(io_uring_warden *s, int f, felspar::source_location loc)
        : completion<void>{s, std::move(loc)}, fd{f} {}
        int fd = {};
        void await_suspend(felspar::coro::coroutine_handle<> h) override {
            handle = h;
            auto sqe = self->ring->next_sqe();
            ::io_uring_prep_poll_add(sqe, fd, POLLOUT);
            ::io_uring_sqe_set_data(sqe, this);
        }
    };
    return {new c{this, fd, std::move(loc)}};
}
