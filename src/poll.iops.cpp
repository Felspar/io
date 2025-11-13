#include "poll.hpp"

#include <felspar/exceptions.hpp>
#include <felspar/io/connect.hpp>


struct felspar::io::poll_warden::close_completion : public completion<void> {
    close_completion(
            poll_warden *s, socket_descriptor f, std::source_location const &loc)
    : completion<void>{s, {}, loc}, fd{f} {}
    socket_descriptor fd;
    void cancel_iop() override {}
    std::coroutine_handle<> try_or_resume() override {
#ifdef FELSPAR_WINSOCK2
        ::closesocket(fd);
#else
        ::close(fd);
#endif
        return cancel_timeout_then_resume();
    }
};
felspar::io::iop<void> felspar::io::poll_warden::do_close(
        socket_descriptor fd, std::source_location const &loc) {
    return {new close_completion{this, fd, loc}};
}


struct felspar::io::poll_warden::sleep_completion : public completion<void> {
    sleep_completion(
            poll_warden *s,
            std::chrono::nanoseconds ns,
            std::source_location const &loc)
    : completion<void>{s, ns, loc} {}
    void cancel_iop() override {}
    std::coroutine_handle<> iop_timedout() override {
        return io::completion<void>::handle;
    }
    std::coroutine_handle<> try_or_resume() override {
        return std::noop_coroutine();
    }
};
felspar::io::iop<void> felspar::io::poll_warden::do_sleep(
        std::chrono::nanoseconds ns, std::source_location const &loc) {
    return {new sleep_completion{this, ns, loc}};
}


struct felspar::io::poll_warden::read_some_completion :
public completion<std::size_t> {
    read_some_completion(
            poll_warden *s,
            socket_descriptor f,
            std::span<std::byte> b,
            std::optional<std::chrono::nanoseconds> timeout,
            std::source_location const &loc)
    : completion<std::size_t>{s, timeout, loc}, fd{f}, buf{b} {}
    socket_descriptor fd;
    std::span<std::byte> buf;
    void cancel_iop() override { std::erase(self->requests[fd].reads, this); }
    std::coroutine_handle<> try_or_resume() override {
#ifdef FELSPAR_WINSOCK2
        if (auto const bytes = recv(
                    fd, reinterpret_cast<char *>(buf.data()), buf.size(), {});
            bytes != SOCKET_ERROR) {
#else
        if (auto const bytes = ::read(fd, buf.data(), buf.size()); bytes >= 0) {
#endif
            result = bytes;
            return cancel_timeout_then_resume();
        } else if (auto const error = get_error(); would_block(error)) {
            self->requests[fd].reads.push_back(this);
            return std::noop_coroutine();
        } else {
            result = {{error, std::system_category()}, "read"};
            return cancel_timeout_then_resume();
        }
    }
};
felspar::io::iop<std::size_t> felspar::io::poll_warden::do_read_some(
        socket_descriptor fd,
        std::span<std::byte> buf,
        std::optional<std::chrono::nanoseconds> timeout,
        std::source_location const &loc) {
    return {new read_some_completion{this, fd, buf, timeout, loc}};
}


struct felspar::io::poll_warden::write_some_completion :
public completion<std::size_t> {
    write_some_completion(
            poll_warden *s,
            socket_descriptor f,
            std::span<std::byte const> b,
            std::optional<std::chrono::nanoseconds> t,
            std::source_location const &loc)
    : completion<std::size_t>{s, t, loc}, fd{f}, buf{b} {}
    socket_descriptor fd;
    std::span<std::byte const> buf;
    void cancel_iop() override { std::erase(self->requests[fd].writes, this); }
    std::coroutine_handle<> try_or_resume() override {
#ifdef FELSPAR_WINSOCK2
        if (auto const bytes =
                    ::send(fd, reinterpret_cast<char const *>(buf.data()),
                           buf.size(), {});
            bytes != SOCKET_ERROR) {
#else
        if (auto const bytes = ::write(fd, buf.data(), buf.size());
            bytes >= 0) {
#endif
            result = bytes;
            return cancel_timeout_then_resume();
        } else if (auto const error = get_error(); would_block(error)) {
            self->requests[fd].writes.push_back(this);
            return std::noop_coroutine();
        } else {
            result = {{error, std::system_category()}, "write"};
            return cancel_timeout_then_resume();
        }
    }
};
felspar::io::iop<std::size_t> felspar::io::poll_warden::do_write_some(
        socket_descriptor fd,
        std::span<std::byte const> buf,
        std::optional<std::chrono::nanoseconds> t,
        std::source_location const &loc) {
    return {new write_some_completion{this, fd, buf, t, loc}};
}


struct felspar::io::poll_warden::accept_completion :
public completion<socket_descriptor> {
    accept_completion(
            poll_warden *s,
            socket_descriptor f,
            std::optional<std::chrono::nanoseconds> t,
            std::source_location const &loc)
    : completion<socket_descriptor>{s, t, loc}, fd{f} {}
    socket_descriptor fd;
    void cancel_iop() override { std::erase(self->requests[fd].reads, this); }
    std::coroutine_handle<> try_or_resume() override {
#ifdef FELSPAR_WINSOCK2
        if (auto const r = ::accept(fd, nullptr, nullptr);
            r != INVALID_SOCKET) {
#elif defined(FELSPAR_HAS_ACCEPT4)
        if (auto const r = ::accept4(fd, nullptr, nullptr, SOCK_NONBLOCK);
            r >= 0) {
#else
        if (auto const r = ::accept(fd, nullptr, nullptr); r >= 0) {
            posix::set_non_blocking(r, loc);
#endif
            result = r;
            return cancel_timeout_then_resume();
        } else if (auto const error = get_error(); would_block(error)) {
            self->requests[fd].reads.push_back(this);
            return std::noop_coroutine();
        } else if (bad_fd(error)) {
            result = r;
            return cancel_timeout_then_resume();
        } else {
            result = {{error, std::system_category()}, "accept"};
            return cancel_timeout_then_resume();
        }
    }
};
felspar::io::iop<felspar::io::socket_descriptor>
        felspar::io::poll_warden::do_accept(
                socket_descriptor fd,
                std::optional<std::chrono::nanoseconds> timeout,
                std::source_location const &loc) {
    return {new accept_completion{this, fd, timeout, loc}};
}


struct felspar::io::poll_warden::connect_completion : public completion<void> {
    connect_completion(
            poll_warden *s,
            socket_descriptor f,
            sockaddr const *a,
            socklen_t l,
            std::optional<std::chrono::nanoseconds> t,
            std::source_location const &loc)
    : completion<void>{s, t, loc}, fd{f}, addr{a}, addrlen{l} {}
    socket_descriptor fd;
    sockaddr const *addr;
    socklen_t addrlen;
    void cancel_iop() override { std::erase(self->requests[fd].writes, this); }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) override {
        handle = h;
#ifdef FELSPAR_WINSOCK2
        if (auto err = ::connect(fd, addr, addrlen); err != SOCKET_ERROR) {
            return handle;
        } else if (auto const wsae = WSAGetLastError(); would_block(wsae)) {
            self->requests[fd].writes.push_back(this);
            insert_timeout();
            return std::noop_coroutine();
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
            return std::noop_coroutine();
        } else {
            result = {
                    {errno, std::system_category()},
                    "connect failure in initial call"};
            return handle;
        }
#endif
    }
    std::coroutine_handle<> try_or_resume() override {
        int errvalue{};
#if defined(FELSPAR_WINSOCK2)
        char *perr = reinterpret_cast<char *>(&errvalue);
#else
        int *perr = &errvalue;
#endif
        ::socklen_t length{sizeof(errvalue)};
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, perr, &length) == 0) {
            if (errvalue == 0) {
                return cancel_timeout_then_resume();
            } else if (errno == EINPROGRESS) {
                self->requests[fd].writes.push_back(this);
                insert_timeout();
                return std::noop_coroutine();
            } else {
                result = {
                        {get_error(), std::system_category()},
                        "connect in follow up call"};
                return cancel_timeout_then_resume();
            }
        } else {
            result = {
                    {get_error(), std::system_category()},
                    "connect/getsockopt"};
            return cancel_timeout_then_resume();
        }
    }
};
felspar::io::iop<void> felspar::io::poll_warden::do_connect(
        socket_descriptor fd,
        sockaddr const *addr,
        socklen_t addrlen,
        std::optional<std::chrono::nanoseconds> timeout,
        std::source_location const &loc) {
    return {new connect_completion{this, fd, addr, addrlen, timeout, loc}};
}


struct felspar::io::poll_warden::read_ready_completion :
public completion<void> {
    read_ready_completion(
            poll_warden *s,
            socket_descriptor f,
            std::optional<std::chrono::nanoseconds> t,
            std::source_location const &loc)
    : completion<void>{s, t, loc}, fd{f} {}
    socket_descriptor fd;
    void cancel_iop() override { std::erase(self->requests[fd].reads, this); }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) override {
        handle = h;
        self->requests[fd].reads.push_back(this);
        insert_timeout();
        return std::noop_coroutine();
    }
    std::coroutine_handle<> try_or_resume() override {
        return cancel_timeout_then_resume();
    }
};
felspar::io::iop<void> felspar::io::poll_warden::do_read_ready(
        socket_descriptor fd,
        std::optional<std::chrono::nanoseconds> timeout,
        std::source_location const &loc) {
    return {new read_ready_completion{this, fd, timeout, loc}};
}


struct felspar::io::poll_warden::write_ready_completion :
public completion<void> {
    write_ready_completion(
            poll_warden *s,
            socket_descriptor f,
            std::optional<std::chrono::nanoseconds> t,
            std::source_location const &loc)
    : completion<void>{s, t, loc}, fd{f} {}
    socket_descriptor fd;
    void cancel_iop() override { std::erase(self->requests[fd].writes, this); }
    std::coroutine_handle<>
            await_suspend(std::coroutine_handle<> h) noexcept override {
        handle = h;
        self->requests[fd].writes.push_back(this);
        insert_timeout();
        return std::noop_coroutine();
    }
    std::coroutine_handle<> try_or_resume() override {
        return cancel_timeout_then_resume();
    }
};
felspar::io::iop<void> felspar::io::poll_warden::do_write_ready(
        socket_descriptor fd,
        std::optional<std::chrono::nanoseconds> timeout,
        std::source_location const &loc) {
    return {new write_ready_completion{this, fd, timeout, loc}};
}
