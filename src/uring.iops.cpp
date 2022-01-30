#include "uring.hpp"

#include <poll.h>

#include <iostream>


struct felspar::io::uring_warden::sleep_completion : public completion<void> {
    sleep_completion(
            uring_warden *s,
            std::chrono::nanoseconds ns,
            felspar::source_location loc)
    : completion<void>{s, std::move(loc)}, ts{{}, ns.count()} {}
    __kernel_timespec ts;
    void await_suspend(felspar::coro::coroutine_handle<> h) override {
        auto sqe = setup_submission(h);
        ::io_uring_prep_timeout(sqe, &ts, 0, 0);
        ::io_uring_sqe_set_data(sqe, this);
    }
    void deliver(int result) override {
        completion<void>::deliver(result == -ETIME ? 0 : result);
    }
};
felspar::io::iop<void> felspar::io::uring_warden::sleep(
        std::chrono::nanoseconds ns, felspar::source_location loc) {
    return {new sleep_completion{this, ns, std::move(loc)}};
}


struct felspar::io::uring_warden::read_some_completion :
public completion<std::size_t> {
    read_some_completion(
            uring_warden *s,
            int f,
            std::span<std::byte> b,
            felspar::source_location loc)
    : completion<std::size_t>{s, std::move(loc)}, fd{f}, bytes{b} {}
    int fd;
    std::span<std::byte> bytes;
    void await_suspend(felspar::coro::coroutine_handle<> h) override {
        auto sqe = setup_submission(h);
        ::io_uring_prep_read(sqe, fd, bytes.data(), bytes.size(), 0);
        ::io_uring_sqe_set_data(sqe, this);
    }
};
felspar::io::iop<std::size_t> felspar::io::uring_warden::read_some(
        int fd, std::span<std::byte> b, felspar::source_location loc) {
    return {new read_some_completion{this, fd, b, std::move(loc)}};
}


struct felspar::io::uring_warden::write_some_completion :
public completion<std::size_t> {
    write_some_completion(
            uring_warden *s,
            int f,
            std::span<std::byte const> b,
            felspar::source_location loc)
    : completion<std::size_t>{s, std::move(loc)}, fd{f}, bytes{b} {}
    int fd;
    std::span<std::byte const> bytes;
    void await_suspend(felspar::coro::coroutine_handle<> h) override {
        auto sqe = setup_submission(h);
        ::io_uring_prep_write(sqe, fd, bytes.data(), bytes.size(), 0);
        ::io_uring_sqe_set_data(sqe, this);
    }
};
felspar::io::iop<std::size_t> felspar::io::uring_warden::write_some(
        int fd, std::span<std::byte const> b, felspar::source_location loc) {
    return {new write_some_completion{this, fd, b, std::move(loc)}};
}


struct felspar::io::uring_warden::accept_completion : public completion<int> {
    accept_completion(uring_warden *s, int f, felspar::source_location loc)
    : completion<int>{s, std::move(loc)}, fd{f} {}
    int fd = {};
    sockaddr addr = {};
    socklen_t addrlen = {};
    void await_suspend(felspar::coro::coroutine_handle<> h) override {
        auto sqe = setup_submission(h);
        ::io_uring_prep_accept(sqe, fd, &addr, &addrlen, 0);
        ::io_uring_sqe_set_data(sqe, this);
    }
};
felspar::io::iop<int> felspar::io::uring_warden::accept(
        int fd, felspar::source_location loc) {
    return {new accept_completion{this, fd, std::move(loc)}};
}


struct felspar::io::uring_warden::connect_completion : public completion<void> {
    connect_completion(
            uring_warden *s,
            int f,
            sockaddr const *a,
            socklen_t l,
            felspar::source_location loc)
    : completion<void>{s, std::move(loc)}, fd{f}, addr{a}, addrlen{l} {}
    int fd = {};
    sockaddr const *addr = {};
    socklen_t addrlen = {};
    void await_suspend(felspar::coro::coroutine_handle<> h) override {
        auto sqe = setup_submission(h);
        ::io_uring_prep_connect(sqe, fd, addr, addrlen);
        ::io_uring_sqe_set_data(sqe, this);
    }
};
felspar::io::iop<void> felspar::io::uring_warden::connect(
        int fd,
        sockaddr const *addr,
        socklen_t addrlen,
        felspar::source_location loc) {
    return {new connect_completion{this, fd, addr, addrlen, std::move(loc)}};
}


struct felspar::io::uring_warden::poll_completion : public completion<void> {
    poll_completion(
            uring_warden *s, int f, short m, felspar::source_location loc)
    : completion<void>{s, std::move(loc)}, fd{f}, mask{m} {}
    int fd = {};
    short mask = {};
    void await_suspend(felspar::coro::coroutine_handle<> h) override {
        auto sqe = setup_submission(h);
        ::io_uring_prep_poll_add(sqe, fd, mask);
        ::io_uring_sqe_set_data(sqe, this);
    }
};
felspar::io::iop<void> felspar::io::uring_warden::read_ready(
        int fd, felspar::source_location loc) {
    return {new poll_completion{this, fd, POLLIN, std::move(loc)}};
}
felspar::io::iop<void> felspar::io::uring_warden::write_ready(
        int fd, felspar::source_location loc) {
    return {new poll_completion{this, fd, POLLOUT, std::move(loc)}};
}
