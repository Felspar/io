#include <felspar/io.hpp>
#include <felspar/test.hpp>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


using namespace std::literals;


namespace {


    auto const suite = felspar::testsuite("connect");


    felspar::coro::task<void> timed_connect(felspar::io::warden &ward) {
        felspar::test::injected check;

        struct addrinfo hints = {};
        hints.ai_socktype = SOCK_STREAM;
        struct addrinfo *addresses = nullptr;
        check(getaddrinfo("www.felspar.com", nullptr, &hints, &addresses)) == 0;
        check(addresses) != nullptr;

        sockaddr_in address =
                *reinterpret_cast<sockaddr_in *>(addresses->ai_addr);
        address.sin_port = ::htons(808);
        freeaddrinfo(addresses);

        auto fd = ward.create_socket(AF_INET, SOCK_STREAM, 0);
        try {
            co_await ward.connect(
                    fd, reinterpret_cast<sockaddr const *>(&address),
                    sizeof(address), 10ms);
            check(false) == true;
        } catch (felspar::io::timeout const &) {
            check(true) == true;
        } catch (...) { check(false) == true; }
    }
    auto const p = suite.test("poll", []() {
        felspar::io::poll_warden ward;
        ward.run(timed_connect);
    });
    auto const u = suite.test("uring", []() {
        felspar::io::uring_warden ward{5};
        ward.run(timed_connect);
    });


}
