#include <felspar/exceptions.hpp>
#include <felspar/poll/accept.hpp>
#include <felspar/poll/connect.hpp>
#include <felspar/poll/executor.hpp>
#include <felspar/poll/read.hpp>
#include <felspar/poll/write.hpp>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>


felspar::coro::stream<int> felspar::poll::accept(executor &exec, int fd) {
    while (true) {
        if (int cnx = ::accept(fd, nullptr, nullptr); cnx >= 0) {
            co_yield cnx;
        } else if (errno == EWOULDBLOCK or errno == EAGAIN) {
            co_await exec.read(fd);
        } else if (errno == EBADF) {
            co_return;
        } else {
            throw felspar::stdexcept::system_error{
                    errno, std::generic_category(), "accept"};
        }
    }
}


felspar::coro::task<void> felspar::poll::connect(
        executor &exec, int fd, const struct sockaddr *addr, socklen_t addrlen) {
    if (auto err = ::connect(fd, addr, addrlen); err == 0) {
        co_return;
    } else if (errno == EINPROGRESS) {
        co_await exec.write(fd);
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
        executor &exec, int fd, void *buf, std::size_t count) {
    while (true) {
        if (auto bytes = ::read(fd, buf, count); bytes >= 0) {
            co_return bytes;
        } else if (errno == EAGAIN or errno == EWOULDBLOCK) {
            co_await exec.read(fd);
        } else {
            throw felspar::stdexcept::system_error{
                    errno, std::generic_category(), "read"};
        }
    }
}


felspar::coro::task<::ssize_t> felspar::poll::write(
        executor &exec, int fd, void const *buf, std::size_t count) {
    while (true) {
        if (auto bytes = ::write(fd, buf, count); bytes >= 0) {
            co_return bytes;
        } else if (errno == EAGAIN or errno == EWOULDBLOCK) {
            co_await exec.write(fd);
        } else {
            throw felspar::stdexcept::system_error{
                    errno, std::generic_category(), "write"};
        }
    }
}
