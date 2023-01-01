#include <felspar/io.hpp>
#include <felspar/coro/eager.hpp>
#include <felspar/coro/start.hpp>
#include <felspar/test.hpp>


using namespace std::literals;


namespace {


    auto const suite = felspar::testsuite("basics");


    felspar::io::warden::task<void> echo_connection(
            felspar::io::warden &ward, felspar::posix::fd sock) {
        std::array<std::byte, 256> buffer;
        while (auto bytes = co_await ward.read_some(sock, buffer, 20ms)) {
            std::span writing{buffer};
            [[maybe_unused]] auto written = co_await felspar::io::write_all(
                    ward, sock, writing.first(bytes), 20ms);
        }
    }

    felspar::io::warden::task<void>
            echo_server(felspar::io::warden &ward, std::uint16_t port) {
        auto fd = ward.create_socket(AF_INET, SOCK_STREAM, 0);
        felspar::posix::set_reuse_port(fd);
        felspar::posix::bind_to_any_address(fd, port);

        int constexpr backlog = 64;
        felspar::posix::listen(fd, backlog);

        felspar::io::warden::starter<void> co;
        for (auto acceptor = felspar::io::accept(ward, fd);
             auto cnx = co_await acceptor.next();) {
            co.post(echo_connection, ward, felspar::posix::fd{*cnx});
            co.garbage_collect_completed();
        }
    }

    felspar::io::warden::task<void>
            echo_client(felspar::io::warden &ward, std::uint16_t port) {
        felspar::test::injected check;

        auto fd = ward.create_socket(AF_INET, SOCK_STREAM, 0);

        sockaddr_in in;
        in.sin_family = AF_INET;
        in.sin_port = htons(port);
        in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        co_await ward.connect(
                fd, reinterpret_cast<sockaddr const *>(&in), sizeof(in));

        std::array<std::uint8_t, 6> out{1, 2, 3, 4, 5, 6}, buffer{};
        co_await felspar::io::write_all(ward, fd, out, 20ms);

        auto bytes = co_await felspar::io::read_exactly(ward, fd, buffer, 20ms);
        check(bytes) == 6u;
        check(buffer[0]) == out[0];
        check(buffer[1]) == out[1];
        check(buffer[2]) == out[2];
        check(buffer[3]) == out[3];
        check(buffer[4]) == out[4];
        check(buffer[5]) == out[5];

        /// Check read time out
        try {
            co_await felspar::io::read_exactly(ward, fd, buffer, 10ms);
            check(false) == true;
        } catch (felspar::io::timeout const &) {
            check(true) == true;
        } catch (std::exception const &e) {
            check(e.what()) == ""; // Print exception `what`
        } catch (...) { check(false) == true; }
        /// Check read_ready time out
        try {
            co_await ward.read_ready(fd, 10ms);
            check(false) == true;
        } catch (felspar::io::timeout const &) {
            check(true) == true;
        } catch (...) { check(false) == true; }
    }


    auto const tp = suite.test("echo/poll", []() {
        felspar::io::poll_warden ward;
        felspar::io::warden::eager<> co;
        co.post(echo_server, ward, 5543);
        ward.run(echo_client, 5543);
    });
#ifdef FELSPAR_ENABLE_IO_URING
    auto const tu = suite.test("echo/uring", []() {
        felspar::io::uring_warden ward{10};
        felspar::io::warden::eager<> co;
        co.post(echo_server, ward, 5547);
        ward.run(echo_client, 5547);
    });
#endif


}
