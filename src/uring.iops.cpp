#include "uring.hpp"

#include <poll.h>

#include <iostream>


struct felspar::io::uring_warden::sleep_completion : public completion<void> {
    sleep_completion(
            uring_warden *s,
            std::chrono::nanoseconds ns,
            felspar::source_location const &loc)
    : completion<void>{s, {}, loc} {
        kts = {{}, ns.count()};
    }
    felspar::coro::coroutine_handle<>
            await_suspend(felspar::coro::coroutine_handle<> h) override {
        auto sqe = setup_submission(h);
        ::io_uring_prep_timeout(sqe, &kts, 0, 0);
        ::io_uring_sqe_set_data(sqe, this);
        return felspar::coro::noop_coroutine();
    }
    void deliver(int result) override {
        completion<void>::deliver(result == -ETIME ? 0 : result);
    }
};
felspar::io::iop<void> felspar::io::uring_warden::do_sleep(
        std::chrono::nanoseconds ns, felspar::source_location const &loc) {
    return {new sleep_completion{this, ns, loc}};
}


struct felspar::io::uring_warden::read_some_completion :
public completion<std::size_t> {
    read_some_completion(
            uring_warden *s,
            int f,
            std::span<std::byte> b,
            std::optional<std::chrono::nanoseconds> t,
            felspar::source_location const &loc)
    : completion<std::size_t>{s, t, loc}, fd{f}, bytes{b} {}
    int fd;
    std::span<std::byte> bytes;
    felspar::coro::coroutine_handle<>
            await_suspend(felspar::coro::coroutine_handle<> h) override {
        auto sqe = setup_submission(h);
        ::io_uring_prep_read(sqe, fd, bytes.data(), bytes.size(), 0);
        return setup_timeout(sqe);
    }
};
felspar::io::iop<std::size_t> felspar::io::uring_warden::do_read_some(
        socket_descriptor fd,
        std::span<std::byte> b,
        std::optional<std::chrono::nanoseconds> timeout,
        felspar::source_location const &loc) {
    return {new read_some_completion{this, fd, b, timeout, loc}};
}


struct felspar::io::uring_warden::write_some_completion :
public completion<std::size_t> {
    write_some_completion(
            uring_warden *s,
            socket_descriptor f,
            std::span<std::byte const> b,
            std::optional<std::chrono::nanoseconds> t,
            felspar::source_location const &loc)
    : completion<std::size_t>{s, t, loc}, fd{f}, bytes{b} {}
    socket_descriptor fd;
    std::span<std::byte const> bytes;
    felspar::coro::coroutine_handle<>
            await_suspend(felspar::coro::coroutine_handle<> h) override {
        auto sqe = setup_submission(h);
        ::io_uring_prep_write(sqe, fd, bytes.data(), bytes.size(), 0);
        return setup_timeout(sqe);
    }
};
felspar::io::iop<std::size_t> felspar::io::uring_warden::do_write_some(
        socket_descriptor fd,
        std::span<std::byte const> b,
        std::optional<std::chrono::nanoseconds> t,
        felspar::source_location const &loc) {
    return {new write_some_completion{this, fd, b, t, loc}};
}


struct felspar::io::uring_warden::accept_completion :
public completion<socket_descriptor> {
    accept_completion(
            uring_warden *s,
            socket_descriptor f,
            std::optional<std::chrono::nanoseconds> t,
            felspar::source_location const &loc)
    : completion<socket_descriptor>{s, t, loc}, fd{f} {}
    socket_descriptor fd = {};
    sockaddr addr = {};
    socklen_t addrlen = {};
    felspar::coro::coroutine_handle<>
            await_suspend(felspar::coro::coroutine_handle<> h) override {
        auto sqe = setup_submission(h);
        ::io_uring_prep_accept(sqe, fd, &addr, &addrlen, 0);
        return setup_timeout(sqe);
    }
};
felspar::io::iop<felspar::io::socket_descriptor>
        felspar::io::uring_warden::do_accept(
                socket_descriptor fd,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &loc) {
    return {new accept_completion{this, fd, timeout, loc}};
}


struct felspar::io::uring_warden::connect_completion : public completion<void> {
    connect_completion(
            uring_warden *s,
            socket_descriptor f,
            sockaddr const *a,
            socklen_t l,
            std::optional<std::chrono::nanoseconds> t,
            felspar::source_location const &loc)
    : completion<void>{s, t, loc}, fd{f}, addr{a}, addrlen{l} {}
    socket_descriptor fd = {};
    sockaddr const *addr = {};
    socklen_t addrlen = {};
    felspar::coro::coroutine_handle<>
            await_suspend(felspar::coro::coroutine_handle<> h) override {
        auto sqe = setup_submission(h);
        ::io_uring_prep_connect(sqe, fd, addr, addrlen);
        return setup_timeout(sqe);
    }
};
felspar::io::iop<void> felspar::io::uring_warden::do_connect(
        socket_descriptor fd,
        sockaddr const *addr,
        socklen_t addrlen,
        std::optional<std::chrono::nanoseconds> timeout,
        felspar::source_location const &loc) {
    return {new connect_completion{this, fd, addr, addrlen, timeout, loc}};
}


struct felspar::io::uring_warden::poll_completion : public completion<void> {
    poll_completion(
            uring_warden *s,
            socket_descriptor f,
            short m,
            std::optional<std::chrono::nanoseconds> t,
            felspar::source_location const &loc)
    : completion<void>{s, t, loc}, fd{f}, mask{m} {}
    socket_descriptor fd = {};
    short mask = {};
    felspar::coro::coroutine_handle<>
            await_suspend(felspar::coro::coroutine_handle<> h) override {
        auto sqe = setup_submission(h);
        ::io_uring_prep_poll_add(sqe, fd, mask);
        return setup_timeout(sqe);
    }
};
felspar::io::iop<void> felspar::io::uring_warden::do_read_ready(
        socket_descriptor fd,
        std::optional<std::chrono::nanoseconds> timeout,
        felspar::source_location const &loc) {
    return {new poll_completion{this, fd, POLLIN, timeout, loc}};
}
felspar::io::iop<void> felspar::io::uring_warden::do_write_ready(
        socket_descriptor fd,
        std::optional<std::chrono::nanoseconds> timeout,
        felspar::source_location const &loc) {
    return {new poll_completion{this, fd, POLLOUT, timeout, loc}};
}
