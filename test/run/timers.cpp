#include <felspar/io.hpp>
#include <felspar/coro/eager.hpp>
#include <felspar/test.hpp>


using namespace std::literals;


namespace {


    auto const suite = felspar::testsuite("timers");


    felspar::io::warden::task<bool> short_sleep(felspar::io::warden &ward) {
        auto const start = std::chrono::steady_clock::now();
        co_await ward.sleep(20ms);
        auto const slept = std::chrono::steady_clock::now() - start;
        /// We have to take sleep jitter into a/c which could be 1ms in the
        /// wrong direction
        co_return slept >= 19ms and slept <= 80ms;
    }
    auto const ssp = suite.test("timers/poll", [](auto check) {
        felspar::io::poll_warden ward;
        check(ward.run(short_sleep)) == true;
    });
#ifdef FELSPAR_ENABLE_IO_URING
    auto const ssu = suite.test("timers/uring", [](auto check) {
        felspar::io::uring_warden ward{5};
        check(ward.run(short_sleep)) == true;
    });
#endif


    felspar::io::warden::task<void>
            accept_writer(felspar::io::warden &ward, std::uint16_t port) {
        auto fd = ward.create_socket(AF_INET, SOCK_STREAM, 0);
        felspar::posix::set_reuse_port(fd);
        felspar::posix::bind_to_any_address(fd, port);
        felspar::posix::listen(fd, 64);

        auto acceptor = felspar::io::accept(ward, fd);
        auto cnx = co_await acceptor.next();
        co_await ward.sleep(30ms);
    }
    felspar::io::warden::task<void>
            write_forever(felspar::io::warden &ward, std::uint16_t port) {
        felspar::test::injected check;

        std::array<std::byte, 1 << 10> buffer;
        auto fd = ward.create_socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in in;
        in.sin_family = AF_INET;
        in.sin_port = htons(port);
        in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        co_await ward.connect(
                fd, reinterpret_cast<sockaddr const *>(&in), sizeof(in));
        try {
            while (co_await felspar::io::write_some(ward, fd, buffer, 10ms));
            check(false) == true;
        } catch (felspar::io::timeout const &) {
            check(true) == true;
        } catch (...) { check(false) == true; }
        try {
            co_await ward.write_ready(fd, 10ms);
            check(false) == true;
        } catch (felspar::io::timeout const &) {
            check(true) == true;
        } catch (...) { check(false) == true; }

        while (true) {
            auto const result{co_await felspar::io::ec{
                    felspar::io::write_some(ward, fd, buffer, 10ms)}};
            if (not result and result.error == felspar::io::timeout::error) {
                co_return;
            } else if (not result) {
                check(result.error)
                        == std::error_code{}; // Force print result.error
            }
        }
    }
    auto const wp = suite.test("write/poll", []() {
        felspar::io::poll_warden ward;
        felspar::io::warden::eager<> co;
        co.post(accept_writer, ward, 5534);
        ward.run(write_forever, 5534);
    });
#ifdef FELSPAR_ENABLE_IO_URING
    auto const wu = suite.test("write/io_uring", []() {
        felspar::io::uring_warden ward;
        felspar::io::warden::eager<> co;
        co.post(accept_writer, ward, 5536);
        ward.run(write_forever, 5536);
    });
#endif


    felspar::io::warden::task<void> short_accept(
            felspar::io::warden &ward,
            std::uint16_t port,
            felspar::test::injected check,
            std::ostream &log) {
        auto fd = ward.create_socket(AF_INET, SOCK_STREAM, 0);
        set_reuse_port(fd);
        bind_to_any_address(fd, port);
        listen(fd, 64);

        try {
            co_await ward.accept(fd, 10ms);
            check(false) == true;
        } catch (felspar::io::timeout const &) {
            check(true) == true;
        } catch (std::exception const &e) {
            log << e.what() << '\n';
            check(false) == true;
        } catch (...) { check(false) == true; }
    }
    auto const ap = suite.test("accept/poll", [](auto check, auto &log) {
        felspar::io::poll_warden ward;
        ward.run(short_accept, 5538, check, std::ref(log));
    });
#ifdef FELSPAR_ENABLE_IO_URING
    auto const au = suite.test("accept/io_uring", [](auto check, auto &log) {
        felspar::io::uring_warden ward;
        ward.run(short_accept, 5540, check, std::ref(log));
    });
#endif


}
