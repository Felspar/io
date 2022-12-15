#include "poll.hpp"

#include <felspar/exceptions.hpp>
#include <felspar/io/connect.hpp>


struct felspar::io::poll_warden::sleep_completion : public completion<void> {
    sleep_completion(
            poll_warden *s,
            std::chrono::nanoseconds ns,
            felspar::source_location const &loc)
    : completion<void>{s, ns, loc} {}
    void cancel_iop() override {}
    felspar::coro::coroutine_handle<> iop_timedout() override {
        return io::completion<void>::handle;
    }
    felspar::coro::coroutine_handle<> try_or_resume() override {
        return felspar::coro::noop_coroutine();
    }
};
felspar::io::iop<void> felspar::io::poll_warden::do_sleep(
        std::chrono::nanoseconds ns, felspar::source_location const &loc) {
    return {new sleep_completion{this, ns, loc}};
}


struct felspar::io::poll_warden::read_some_completion :
public completion<std::size_t> {
    read_some_completion(
            poll_warden *s,
            socket_descriptor f,
            std::span<std::byte> b,
            std::optional<std::chrono::nanoseconds> timeout,
            felspar::source_location const &loc)
    : completion<std::size_t>{s, timeout, loc}, fd{f}, buf{b} {}
    socket_descriptor fd;
    std::span<std::byte> buf;
    void cancel_iop() override { std::erase(self->requests[fd].reads, this); }
    felspar::coro::coroutine_handle<> try_or_resume() override {
        if (auto bytes = ::read(fd, buf.data(), buf.size()); bytes >= 0) {
            result = bytes;
            return cancel_timeout_then_resume();
        } else if (errno == EAGAIN or errno == EWOULDBLOCK) {
            self->requests[fd].reads.push_back(this);
            return felspar::coro::noop_coroutine();
        } else {
            result = {{errno, std::system_category()}, "read"};
            return cancel_timeout_then_resume();
        }
    }
};
felspar::io::iop<std::size_t> felspar::io::poll_warden::do_read_some(
        socket_descriptor fd,
        std::span<std::byte> buf,
        std::optional<std::chrono::nanoseconds> timeout,
        felspar::source_location const &loc) {
    return {new read_some_completion{this, fd, buf, timeout, loc}};
}


struct felspar::io::poll_warden::write_some_completion :
public completion<std::size_t> {
    write_some_completion(
            poll_warden *s,
            socket_descriptor f,
            std::span<std::byte const> b,
            std::optional<std::chrono::nanoseconds> t,
            felspar::source_location const &loc)
    : completion<std::size_t>{s, t, loc}, fd{f}, buf{b} {}
    socket_descriptor fd;
    std::span<std::byte const> buf;
    void cancel_iop() override { std::erase(self->requests[fd].writes, this); }
    felspar::coro::coroutine_handle<> try_or_resume() override {
#ifdef FELSPAR_WINSOCK2
        if (auto const bytes = ::send(fd, reinterpret_cast<char const *>(buf.data()), buf.size(), {}); bytes != SOCKET_ERROR) {
            result = bytes;
            return cancel_timeout_then_resume();
        } else {
            result = {{WSAGetLastError(), std::system_category()}, "write"};
            return cancel_timeout_then_resume();
        }
#else
        if (auto bytes = ::write(fd, buf.data(), buf.size()); bytes >= 0) {
            result = bytes;
            return cancel_timeout_then_resume();
        } else if (errno == EAGAIN or errno == EWOULDBLOCK) {
            self->requests[fd].writes.push_back(this);
            return felspar::coro::noop_coroutine();
        } else {
            result = {{errno, std::system_category()}, "write"};
            return cancel_timeout_then_resume();
        }
#endif
    }
};
felspar::io::iop<std::size_t> felspar::io::poll_warden::do_write_some(
        socket_descriptor fd,
        std::span<std::byte const> buf,
        std::optional<std::chrono::nanoseconds> t,
        felspar::source_location const &loc) {
    return {new write_some_completion{this, fd, buf, t, loc}};
}


struct felspar::io::poll_warden::accept_completion :
public completion<socket_descriptor> {
    accept_completion(
            poll_warden *s,
            socket_descriptor f,
            std::optional<std::chrono::nanoseconds> t,
            felspar::source_location const &loc)
    : completion<socket_descriptor>{s, t, loc}, fd{f} {}
    socket_descriptor fd;
    void cancel_iop() override { std::erase(self->requests[fd].reads, this); }
    felspar::coro::coroutine_handle<> try_or_resume() override {
#ifdef FELSPAR_WINSOCK2
        auto const r = ::accept(fd, nullptr, nullptr);
        if (r != INVALID_SOCKET) {
            result = r;
            return cancel_timeout_then_resume();
        } else {
            result = {{WSAGetLastError(), std::system_category()}, "accept"};
            return cancel_timeout_then_resume();
        }
#else
#ifdef FELSPAR_HAS_ACCEPT4
        auto const r = ::accept4(fd, nullptr, nullptr, SOCK_NONBLOCK);
        if (r >= 0) {
            result = {{errno, std::system_category()}, "accept"};
            return cancel_timeout_then_resume();
#else
        auto const r = ::accept(fd, nullptr, nullptr);
        if (r >= 0) {
            posix::set_non_blocking(r, loc);
#endif
            result = r;
            return cancel_timeout_then_resume();
        } else if (errno == EWOULDBLOCK or errno == EAGAIN) {
            self->requests[fd].reads.push_back(this);
            return felspar::coro::noop_coroutine();
        } else if (errno == EBADF) {
            result = r;
            return cancel_timeout_then_resume();
        } else {
            result = {{errno, std::system_category()}, "accept"};
            return cancel_timeout_then_resume();
        }
#endif
    }
};
felspar::io::iop<felspar::io::socket_descriptor>
        felspar::io::poll_warden::do_accept(
                socket_descriptor fd,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &loc) {
    return {new accept_completion{this, fd, timeout, loc}};
}


struct felspar::io::poll_warden::connect_completion : public completion<void> {
    connect_completion(
            poll_warden *s,
            socket_descriptor f,
            sockaddr const *a,
            socklen_t l,
            std::optional<std::chrono::nanoseconds> t,
            felspar::source_location const &loc)
    : completion<void>{s, t, loc}, fd{f}, addr{a}, addrlen{l} {}
    socket_descriptor fd;
    sockaddr const *addr;
    socklen_t addrlen;
    void cancel_iop() override { std::erase(self->requests[fd].writes, this); }
    felspar::coro::coroutine_handle<>
            await_suspend(felspar::coro::coroutine_handle<> h) override {
        handle = h;
#ifdef FELSPAR_WINSOCK2
        if (auto err = ::connect(fd, addr, addrlen); err != SOCKET_ERROR) {
            return handle;
        } else if (auto const wsae = WSAGetLastError(); wsae == WSAEWOULDBLOCK) {
            self->requests[fd].writes.push_back(this);
            insert_timeout();
            return felspar::coro::noop_coroutine();
        } else {
            result = {{wsae, std::system_category()}, "connect error"};
            return handle;
        }
#else
        if (auto err = ::connect(fd, addr, addrlen); err == 0) {
            return handle;
        } else if (errno == EINPROGRESS) {
            self->requests[fd].writes.push_back(this);
            insert_timeout();
            return felspar::coro::noop_coroutine();
        } else {
            result = {{errno, std::system_category()}, "connect"};
            return handle;
        }
#endif
    }
    felspar::coro::coroutine_handle<> try_or_resume() override {
        int errvalue{};
#if defined(FELSPAR_WINSOCK2)
        char *perr = reinterpret_cast<char *>(&errvalue);
#else
        int *perr = &errvalue;
#endif
        ::socklen_t length{sizeof(perr)};
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, perr, &length) == 0) {
            if (errvalue == 0) {
                return cancel_timeout_then_resume();
            } else {
#if defined(FELSPAR_WINSOCK2)
                result = {{WSAGetLastError(), std::system_category()}, "connect"};
#else
                result = {{errno, std::system_category()}, "connect"};
#endif
                return cancel_timeout_then_resume();
            }
        } else {
#if defined(FELSPAR_WINSOCK2)
            result = {{WSAGetLastError(), std::system_category()}, "connect/getsockopt"};
#else
            result = {{errno, std::system_category()}, "connect/getsockopt"};
#endif
            return cancel_timeout_then_resume();
        }
    }
};
felspar::io::iop<void> felspar::io::poll_warden::do_connect(
        socket_descriptor fd,
        sockaddr const *addr,
        socklen_t addrlen,
        std::optional<std::chrono::nanoseconds> timeout,
        felspar::source_location const &loc) {
    return {new connect_completion{this, fd, addr, addrlen, timeout, loc}};
}


struct felspar::io::poll_warden::read_ready_completion :
public completion<void> {
    read_ready_completion(
            poll_warden *s,
            socket_descriptor f,
            std::optional<std::chrono::nanoseconds> t,
            felspar::source_location const &loc)
    : completion<void>{s, t, loc}, fd{f} {}
    socket_descriptor fd;
    void cancel_iop() override { std::erase(self->requests[fd].reads, this); }
    felspar::coro::coroutine_handle<>
            await_suspend(felspar::coro::coroutine_handle<> h) override {
        handle = h;
        self->requests[fd].reads.push_back(this);
        insert_timeout();
        return felspar::coro::noop_coroutine();
    }
    felspar::coro::coroutine_handle<> try_or_resume() override {
        return cancel_timeout_then_resume();
    }
};
felspar::io::iop<void> felspar::io::poll_warden::do_read_ready(
        socket_descriptor fd,
        std::optional<std::chrono::nanoseconds> timeout,
        felspar::source_location const &loc) {
    return {new read_ready_completion{this, fd, timeout, loc}};
}


struct felspar::io::poll_warden::write_ready_completion :
public completion<void> {
    write_ready_completion(
            poll_warden *s,
            socket_descriptor f,
            std::optional<std::chrono::nanoseconds> t,
            felspar::source_location const &loc)
    : completion<void>{s, t, loc}, fd{f} {}
    socket_descriptor fd;
    void cancel_iop() override { std::erase(self->requests[fd].writes, this); }
    felspar::coro::coroutine_handle<> await_suspend(
            felspar::coro::coroutine_handle<> h) noexcept override {
        handle = h;
        self->requests[fd].writes.push_back(this);
        insert_timeout();
        return felspar::coro::noop_coroutine();
    }
    felspar::coro::coroutine_handle<> try_or_resume() override {
        return cancel_timeout_then_resume();
    }
};
felspar::io::iop<void> felspar::io::poll_warden::do_write_ready(
        socket_descriptor fd,
        std::optional<std::chrono::nanoseconds> timeout,
        felspar::source_location const &loc) {
    return {new write_ready_completion{this, fd, timeout, loc}};
}
