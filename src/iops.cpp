#include <felspar/exceptions.hpp>
#include <felspar/poll/accept.hpp>
#include <felspar/poll/connect.hpp>
#include <felspar/poll/executor.hpp>
#include <felspar/poll/read.hpp>
#include <felspar/poll/write.hpp>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>


felspar::coro::stream<int> felspar::poll::accept(executor &exec, int fd) {
    while (true) {
        std::cout << "Calling accept on FD " << fd << std::endl;
        if (int cnx = ::accept(fd, nullptr, nullptr) >= 0) {
            std::cout << "Got an accept fd " << cnx << std::endl;
            co_yield cnx;
        } else if (errno == EWOULDBLOCK or errno == EAGAIN) {
            std::cout << "No accept now, gotta wait" << std::endl;
            co_await exec.read(fd);
        } else if (errno == EBADF) {
            co_return;
        } else {
            throw felspar::stdexcept::system_error{
                    errno, std::generic_category(), "accept"};
        }
    }
}


felspar::coro::task<void> felspar::poll::connect(executor &exec,
        int fd, const struct sockaddr *addr, socklen_t addrlen) {
    std::cout << "Calling connect for FD " << fd << std::endl;
    if (auto err = ::connect(fd, addr, addrlen) == 0) {
        std::cout << "Connection done" << std::endl;
        co_return;
    } else if (errno == EINPROGRESS) {
        std::cout << "connect EINPROGRESS" << std::endl;
        co_await exec.write(fd);
        std::cout << "connect write ready" << std::endl;
        int errvalue{};
        ::socklen_t length{};
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &errvalue, &length) == 0) {
            if (errvalue == 0) {
                std::cout << "connect async complete" << std::endl;
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


felspar::coro::task<::ssize_t> felspar::poll::read(executor &exec, int fd, void *buf, std::size_t count) {
    while (true) {
        std::cout << "Calling read on FD " << fd << std::endl;
        if (auto bytes = ::read(fd, buf, count) >= 0) {
            std::cout << "read done" << std::endl;
            co_return bytes;
        } else if (errno == EAGAIN or errno == EWOULDBLOCK) {
            std::cout << "read errno == EAGAIN or errno == EWOULDBLOCK" << std::endl;
            co_await exec.read(fd);
        } else {
            throw felspar::stdexcept::system_error{
                    errno, std::generic_category(), "read"};
        }
    }
}
