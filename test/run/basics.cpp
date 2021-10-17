#include <felspar/exceptions.hpp>
#include <felspar/poll.hpp>
#include <felspar/test.hpp>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>


namespace {


    auto const suite = felspar::testsuite("basics");


    void set_non_blocking(int fd) {
        if (int err = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
            err != 0) {
            throw felspar::stdexcept::system_error{
                    errno, std::generic_category(),
                    "fcntl F_SETFL unknown error"};
        }
    }


    felspar::coro::task<void> echo_connection(felspar::poll::executor &exec, int fd) {
            set_non_blocking(fd);
            std::array<std::byte, 256> buffer;
            auto bytes = co_await felspar::poll::read(exec, fd, buffer);
            ::close(fd);
    }

    felspar::coro::task<void>
            echo_server(felspar::poll::executor &exec, std::uint16_t port) {
        auto fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            throw felspar::stdexcept::system_error{
                    errno, std::generic_category(), "Creating server socket"};
        }
        set_non_blocking(fd);

        sockaddr_in in;
        in.sin_family = AF_INET;
        in.sin_port = htons(port);
        in.sin_addr.s_addr = htonl(INADDR_ANY);
        if (::bind(fd, reinterpret_cast<sockaddr const *>(&in), sizeof(in))
            != 0) {
            throw felspar::stdexcept::system_error{
                    errno, std::generic_category(), "Binding server socket"};
        }

        int constexpr backlog = 64;
        if (::listen(fd, backlog) == -1) {
            throw felspar::stdexcept::system_error{
                    errno, std::generic_category(), "Calling listen"};
        }

        for (auto acceptor = felspar::poll::accept(exec, fd);
             auto cnx = co_await acceptor.next();) {
            exec.post(echo_connection, *cnx);
        }

        ::close(fd);
    }

    felspar::coro::task<void>
            echo_client(felspar::poll::executor &exec, std::uint16_t port) {
        auto fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            throw felspar::stdexcept::system_error{
                    errno, std::generic_category(), "Creating client socket"};
        }
        set_non_blocking(fd);

        sockaddr_in in;
        in.sin_family = AF_INET;
        in.sin_port = htons(port);
        in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        co_await felspar::poll::connect(exec,
                fd, reinterpret_cast<sockaddr const *>(&in), sizeof(in));

        std::cout << "Client done" << std::endl;
    }

    auto const trans = suite.test("transfer", []() {
        std::cout << "Starting" << std::endl;
        felspar::poll::executor exec;
        exec.post(echo_server, 5543);
        exec.run(echo_client, 5543);
    });


}
