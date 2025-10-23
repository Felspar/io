#include <felspar/io.hpp>
#include <felspar/test.hpp>


using namespace std::literals;


namespace {


    auto const suite = felspar::testsuite("connect");


    felspar::io::warden::task<void> timed_connect(
            felspar::io::warden &ward,
            char const *const hostname,
            felspar::source_location const loc) {
        felspar::test::injected check;

        auto addresses = felspar::io::addrinfo(hostname, 80);
        auto address = *addresses.next();

        try {
            auto fd1 = ward.create_socket(
                    address.first->sa_family, SOCK_STREAM, 0);
            co_await ward.connect(fd1, address.first, address.second, 5s);

            felspar::posix::set_port(*address.first, 808);
            auto fd2 = ward.create_socket(
                    address.first->sa_family, SOCK_STREAM, 0);
            co_await ward.connect(fd2, address.first, address.second, 10ms);

            check(false) == true;
        } catch (felspar::io::timeout const &) {
            check(true) == true;
        } catch (std::exception const &e) {
            check(e.what()) == ""; /// Print out the exception message
        } catch (...) { check(false) == true; }
    }
    auto const p = suite.test(
            "poll",
            []() {
                felspar::io::poll_warden ward;
                ward.run(
                        timed_connect, "felspar.com",
                        std::source_location::current());
            },
            []() {
                felspar::io::poll_warden ward;
                ward.run(
                        timed_connect, "api.blue5alamander.com",
                        std::source_location::current());
            });
#ifdef FELSPAR_ENABLE_IO_URING
    auto const u = suite.test(
            "uring",
            []() {
                felspar::io::uring_warden ward{5};
                ward.run(
                        timed_connect, "api.blue5alamander.com",
                        std::source_location::current());
            });
#endif


}
