#include <felspar/exceptions.hpp>
#include <felspar/poll/accept.hpp>
#include <felspar/poll/executor.hpp>

#include <netinet/in.h>
#include <sys/socket.h>

#include <iostream>


felspar::coro::stream<int> felspar::poll::accept(executor &exec, int fd) {
    while (true) {
        ::sockaddr addr{};
        ::socklen_t addr_len{};
        std::cout << "Calling accept" << std::endl;
        if (int cnx = ::accept(fd, &addr, &addr_len) >= 0) {
            std::cout << "Got an accept fd" << std::endl;
            co_yield cnx;
        } else if (errno == EWOULDBLOCK or errno == EAGAIN) {
            std::cout << "No accept now, gotta wait" << std::endl;
            co_await exec.read(fd);
        } else if (errno == EBADF) {
            co_return;
        } else {
            throw felspar::stdexcept::system_error{
                    errno, std::generic_category(), "Accept"};
        }
    }
}
