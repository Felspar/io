#include <felspar/io.hpp>
#include <felspar/test.hpp>

#include <sys/types.h>
#if defined(FELSPAR_POSIX_SOCKETS)
#include <sys/socket.h>
#include <netdb.h>
#endif


using namespace std::literals;


namespace {


    auto const suite = felspar::testsuite("connect");


    felspar::io::warden::task<void> timed_connect(felspar::io::warden &ward) {
        felspar::test::injected check;

        struct addrinfo hints = {};
        hints.ai_socktype = SOCK_STREAM;
        struct addrinfo *addresses = nullptr;
        check(getaddrinfo("www.felspar.com", nullptr, &hints, &addresses)) == 0;
        check(addresses) != nullptr;

        auto address = *addresses->ai_addr;
        auto address_length = addresses->ai_addrlen;

        freeaddrinfo(addresses);

        try {
            felspar::posix::set_port(address, 80);
            auto fd1 = ward.create_socket(address.sa_family, SOCK_STREAM, 0);
            co_await ward.connect(fd1, &address, address_length, 5s);

            felspar::posix::set_port(address, 808);
            auto fd2 = ward.create_socket(address.sa_family, SOCK_STREAM, 0);
            co_await ward.connect(fd2, &address, address_length, 10ms);

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
