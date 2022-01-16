#include <felspar/exceptions.hpp>
#include <felspar/poll/connect.hpp>
#include <felspar/poll/warden.poll.hpp>

#include <sys/socket.h>
#include <unistd.h>


felspar::poll::iop<int> felspar::poll::poll_warden::accept(int fd) {
    struct c : public completion {
        c(poll_warden *s, int f) : self{s}, fd{f} {}
        poll_warden *self;
        int fd;
        warden *ward() override { return self; }
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


felspar::poll::iop<void> felspar::poll::poll_warden::connect(
        int fd, sockaddr const *addr, socklen_t addrlen) {
    struct c : public completion {
        c(poll_warden *s, int f, sockaddr const *a, socklen_t l)
        : self{s}, fd{f}, addr{a}, addrlen{l} {}
        ~c() = default;
        poll_warden *self;
        int fd;
        sockaddr const *addr;
        socklen_t addrlen;
        warden *ward() override { return self; }
        void await_suspend(felspar::coro::coroutine_handle<> h) override {
            handle = h;
            if (auto err = ::connect(fd, addr, addrlen); err == 0) {
                handle.resume();
            } else if (errno == EINPROGRESS) {
                self->requests[fd].writes.push_back(this);
            } else {
                /// TODO Keep the exception to throw later on when the IOP's
                /// await_resume is executed
                throw felspar::stdexcept::system_error{
                        errno, std::generic_category(), "connect"};
            }
        }
        void try_or_resume() override {
            int errvalue{};
            ::socklen_t length{};
            if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &errvalue, &length) == 0) {
                if (errvalue == 0) {
                    handle.resume();
                } else {
                    /// TODO Keep the exception to throw later on when the IOP's
                    /// await_resume is executed
                    throw felspar::stdexcept::system_error{
                            errno, std::generic_category(), "connect"};
                }
            } else {
                /// TODO Keep the exception to throw later on when the IOP's
                /// await_resume is executed
                throw felspar::stdexcept::system_error{
                        errno, std::generic_category(), "connect/getsockopt"};
            }
        }
    };
    return {new c{this, fd, addr, addrlen}};
}


felspar::poll::iop<void> felspar::poll::poll_warden::read_ready(int fd) {
    struct c : public completion {
        c(poll_warden *s, int f) : self{s}, fd{f} {}
        ~c() = default;
        poll_warden *self;
        int fd;
        warden *ward() override { return self; }
        void await_suspend(felspar::coro::coroutine_handle<> h) override {
            handle = h;
            self->requests[fd].reads.push_back(this);
        }
    };
    return {new c{this, fd}};
}
felspar::poll::iop<void> felspar::poll::poll_warden::write_ready(int fd) {
    struct c : public completion {
        c(poll_warden *s, int f) : self{s}, fd{f} {}
        ~c() = default;
        poll_warden *self;
        int fd;
        warden *ward() override { return self; }
        void await_suspend(
                felspar::coro::coroutine_handle<> h) noexcept override {
            handle = h;
            self->requests[fd].writes.push_back(this);
        }
    };
    return {new c{this, fd}};
}
