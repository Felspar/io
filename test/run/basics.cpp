#include <felspar/exceptions.hpp>
#include <felspar/poll.hpp>
#include <felspar/test.hpp>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>


namespace {


    auto const suite = felspar::testsuite("basics");


    felspar::coro::task<void>
            echo_server(felspar::poll::executor &exec, std::uint16_t port) {
        auto fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            throw felspar::stdexcept::system_error{
                    errno, std::generic_category(), "Creating server socket"};
        }

        sockaddr_in in;
        in.sin_family = AF_INET;
        in.sin_port = htons(port);
        in.sin_addr.s_addr = htonl(INADDR_ANY);
        if (::bind(fd, reinterpret_cast<sockaddr const *>(&in), sizeof(in))
            != 0) {
            throw felspar::stdexcept::system_error{
                    errno, std::generic_category(), "Binding server socket"};
        }

        ::close(fd);
        co_return;
    }

    felspar::coro::task<void>
            echo_client(felspar::poll::executor &exec, std::uint16_t port) {
        auto fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            throw felspar::stdexcept::system_error{
                    errno, std::generic_category(), "Creating client socket"};
        }

        co_return;
    }

    auto const trans = suite.test("transfer", []() {
        felspar::poll::executor exec;
        exec.post(echo_server, 5543);
        exec.run(echo_client, 5543);
    });


}
