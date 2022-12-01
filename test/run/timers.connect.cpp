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
        freeaddrinfo(addresses);

        try {
            address.sin_port = htons(80);
            auto fd1 = ward.create_socket(AF_INET, SOCK_STREAM, 0);
            co_await ward.connect(
                    fd1, reinterpret_cast<sockaddr const *>(&address),
                    sizeof(address), 5s);

            address.sin_port = htons(808);
            auto fd2 = ward.create_socket(AF_INET, SOCK_STREAM, 0);
            co_await ward.connect(
                    fd2, reinterpret_cast<sockaddr const *>(&address),
                    sizeof(address), 10ms);

            check(false) == true;
        } catch (felspar::io::timeout const &) {
            check(true) == true;
        } catch (std::exception const &e) {
            check(e.what()) == ""; /// Print out the exception message
        } catch (...) { check(false) == true; }
    }
    auto const p = suite.test("poll", []() {
        felspar::io::poll_warden ward;
        ward.run(timed_connect);
    });
#ifdef FELSPAR_ENABLE_IO_URING
    auto const u = suite.test("uring", []() {
        felspar::io::uring_warden ward{5};
        ward.run(timed_connect);
    });
#endif


}
