#include <felspar/exceptions.hpp>
#include <felspar/poll/connect.hpp>
#include <felspar/poll/read.hpp>
#include <felspar/poll/warden.poll.hpp>
#include <felspar/poll/write.hpp>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>


felspar::poll::iop<int> felspar::poll::poll_warden::accept(int fd) {
    struct c : public iop<int>::completion {
        c(poll_warden *s, int f) : completion{s}, self{s}, fd{f} {}
        poll_warden *self;
        int fd;
        void try_or_resume() override {
            result = ::accept(fd, nullptr, nullptr);
            if (result >= 0) {
                handle.resume();
            } else if (errno == EWOULDBLOCK or errno == EAGAIN) {
                self->requests[fd].reads.push_back(this);
            } else if (errno == EBADF) {
                handle.resume();
            } else {
                /// TODO Keep the exception to throw later on when the IOP's
                /// await_resume is executed
                throw felspar::stdexcept::system_error{
                        errno, std::generic_category(), "accept"};
            }
        }
    };
    return {new c{this, fd}};
}


felspar::poll::iop<void> felspar::poll::poll_warden::read_ready(int fd) {
    struct c : public iop<void>::completion {
        c(poll_warden *s, int f) : completion{s}, self{s}, fd{f} {}
        ~c() = default;
        poll_warden *self;
        int fd;
        void await_suspend(felspar::coro::coroutine_handle<> h) override {
            handle = h;
            self->requests[fd].reads.push_back(this);
        }
    };
    return {new c{this, fd}};
}
felspar::poll::iop<void> felspar::poll::poll_warden::write_ready(int fd) {
    struct c : public iop<void>::completion {
        c(poll_warden *s, int f) : completion{s}, self{s}, fd{f} {}
        ~c() = default;
        poll_warden *self;
        int fd;
        void await_suspend(
                felspar::coro::coroutine_handle<> h) noexcept override {
            handle = h;
            self->requests[fd].writes.push_back(this);
        }
    };
    return {new c{this, fd}};
}


felspar::coro::task<void> felspar::poll::connect(
        warden &ward, int fd, const struct sockaddr *addr, socklen_t addrlen) {
    if (auto err = ::connect(fd, addr, addrlen); err == 0) {
        co_return;
    } else if (errno == EINPROGRESS) {
        co_await ward.write_ready(fd);
        int errvalue{};
        ::socklen_t length{};
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &errvalue, &length) == 0) {
            if (errvalue == 0) {
                co_return;
            } else {
                throw felspar::stdexcept::system_error{
                        errno, std::generic_category(), "connect"};
            }
        } else {
            throw felspar::stdexcept::system_error{
                    errno, std::generic_category(), "connect/getsockopt"};
        }
    } else {
        throw felspar::stdexcept::system_error{
                errno, std::generic_category(), "connect"};
    }
}


felspar::coro::task<::ssize_t> felspar::poll::read(
        warden &ward, int fd, void *buf, std::size_t count) {
    while (true) {
        if (auto bytes = ::read(fd, buf, count); bytes >= 0) {
            co_return bytes;
        } else if (errno == EAGAIN or errno == EWOULDBLOCK) {
            co_await ward.read_ready(fd);
        } else {
            throw felspar::stdexcept::system_error{
                    errno, std::generic_category(), "read"};
        }
    }
}


felspar::coro::task<::ssize_t> felspar::poll::write(
        warden &ward, int fd, void const *buf, std::size_t count) {
    while (true) {
        if (auto bytes = ::write(fd, buf, count); bytes >= 0) {
            co_return bytes;
        } else if (errno == EAGAIN or errno == EWOULDBLOCK) {
            co_await ward.write_ready(fd);
        } else {
            throw felspar::stdexcept::system_error{
                    errno, std::generic_category(), "write"};
        }
    }
}
