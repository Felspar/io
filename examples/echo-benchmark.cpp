#include <felspar/io.hpp>
#include <felspar/coro/start.hpp>


using namespace std::literals;


namespace {


    felspar::coro::task<void> echo_connection(
            felspar::io::warden &ward, felspar::posix::fd sock) {
        std::array<std::byte, 256> buffer;
        while (auto bytes = co_await ward.read_some(sock, buffer, 20ms)) {
            std::span writing{buffer};
            auto written = co_await felspar::io::write_all(
                    ward, sock, writing.first(bytes), 20ms);
        }
    }
    felspar::coro::task<void>
            echo_server(felspar::io::warden &ward, std::uint16_t port) {
        auto fd = ward.create_socket(AF_INET, SOCK_STREAM, 0);
        felspar::posix::set_reuse_port(fd);
        felspar::posix::bind_to_any_address(fd, port);

        int constexpr backlog = 64;
        if (::listen(fd.native_handle(), backlog) == -1) {
            throw felspar::stdexcept::system_error{
                    errno, std::system_category(), "Calling listen"};
        }

        felspar::coro::starter<felspar::coro::task<void>> co;
        for (auto acceptor = felspar::io::accept(ward, fd);
             auto cnx = co_await acceptor.next();) {
            co.post(echo_connection, ward, felspar::posix::fd{*cnx});
            co.gc();
        }
    }


    felspar::coro::task<void>
            client(felspar::io::warden &ward, std::uint16_t port) {
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
    }


    felspar::coro::task<int> co_main(felspar::io::warden &ward) {
        felspar::coro::starter<felspar::coro::task<void>> server;
        server.post(echo_server, std::ref(ward), 2566);
        co_await ward.sleep(2s);
        co_await client(ward, 2566);
        co_return 0;
    }


}


int main() {
    felspar::io::poll_warden ward;
    felspar::coro::starter<felspar::coro::task<void>> clients;
    return ward.run(co_main);
}
