#include <felspar/poll/accept.hpp>
#include <felspar/poll/read.hpp>
#include <felspar/poll/warden.hpp>
#include <felspar/poll/write.hpp>

#include <felspar/exceptions.hpp>

#include <iostream>


felspar::coro::stream<int> felspar::poll::accept(warden &ward, int fd) {
    while (true) {
        int s = co_await ward.accept(fd);
        if (s >= 0) {
            std::cout << "Accepted socket " << s << std::endl;
            co_yield s;
        } else if (s == -11) {
            felspar::stdexcept::system_error e{
                    -s, std::generic_category(), "accept"};
            std::cout << "About to throw " << e.what() << std::endl;
            throw e;
        } else {
            std::cout << "Accept stopped because " << s << std::endl;
            co_return;
        }
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
