#include <felspar/io.hpp>
#include <felspar/coro/start.hpp>

#include <atomic>
#include <iostream>
#include <thread>


using namespace std::literals;


namespace {


    std::uint16_t g_port = 2556;
    std::atomic<std::uint64_t> server_count_started{}, server_count_completed{};


    felspar::coro::task<void> echo_connection(
            felspar::io::warden &ward, felspar::posix::fd sock) {
        ++server_count_started;
        std::array<std::byte, 256> buffer;
        while (auto bytes = co_await felspar::io::ec(
                       ward.read_some(sock, buffer, 20ms))) {
            std::span writing{buffer};
            auto written = co_await felspar::io::write_all(
                    ward, sock, writing.first(bytes.value()), 20ms);
        }
        ++server_count_completed;
    }
    felspar::coro::task<void> echo_server(felspar::io::warden &ward) {
        auto fd = ward.create_socket(AF_INET, SOCK_STREAM, 0);
        felspar::posix::set_reuse_port(fd);
        felspar::posix::bind_to_any_address(fd, g_port);

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
    felspar::coro::task<void> server_manager(
            felspar::io::warden &ward, felspar::posix::fd control) {
        felspar::coro::starter<felspar::coro::task<void>> server;
        server.post(echo_server, std::ref(ward));
        std::cout << "Echo server running, waiting for end signal "
                  << control.native_handle() << std::endl;
        std::array<std::byte, 8> buffer;
        co_await ward.read_some(control, buffer);
        std::cout << "Server has seen end signal, terminating" << std::endl;
    }


    felspar::coro::task<void> client(felspar::io::warden &ward) {
        auto fd = ward.create_socket(AF_INET, SOCK_STREAM, 0);

        sockaddr_in in;
        in.sin_family = AF_INET;
        in.sin_port = htons(g_port);
        in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        co_await ward.connect(
                fd, reinterpret_cast<sockaddr const *>(&in), sizeof(in));

        std::array<std::uint8_t, 6> out{1, 2, 3, 4, 5, 6}, buffer{};
        co_await felspar::io::write_all(ward, fd, out, 20ms);

        auto bytes = co_await felspar::io::read_exactly(ward, fd, buffer, 20ms);
    }


    felspar::coro::task<int> co_main(felspar::io::warden &ward) {
        auto control_pipe = ward.create_pipe();
        std::thread server{
                [](felspar::posix::fd control) {
                    felspar::io::poll_warden ward;
                    ward.run(server_manager, std::move(control));
                },
                std::move(control_pipe.read)};
        co_await ward.sleep(2s);
        co_await client(ward);
        std::cout << "Test done, signalling server to stop "
                  << control_pipe.write.native_handle() << std::endl;
        std::array<std::byte, 1> signal{std::byte{'x'}};
        co_await ward.write_some(control_pipe.write, signal);
        std::cout << "Waiting for server thread to end" << std::endl;
        server.join();
        std::cout << "Server count started " << server_count_started
                  << " completed " << server_count_completed << '\n';
        co_return 0;
    }


}


int main() {
    felspar::io::poll_warden ward;
    felspar::coro::starter<felspar::coro::task<void>> clients;
    return ward.run(co_main);
}
