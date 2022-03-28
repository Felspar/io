#include <felspar/io.hpp>
#include <felspar/coro/cancellable.hpp>
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
        while (auto bytes = co_await ward.read_some(sock, buffer, 20ms)) {
            std::span writing{buffer};
            auto written = co_await felspar::io::write_all(
                    ward, sock, writing.first(bytes), 20ms);
        }
        ++server_count_completed;
    }
    felspar::coro::task<void> echo_server(
            felspar::io::warden &ward,
            felspar::coro::cancellable &cancellation) {
        auto fd = ward.create_socket(AF_INET, SOCK_STREAM, 0);
        felspar::posix::set_reuse_port(fd);
        felspar::posix::bind_to_any_address(fd, g_port);

        int constexpr backlog = 64;
        if (::listen(fd.native_handle(), backlog) == -1) {
            throw felspar::stdexcept::system_error{
                    errno, std::system_category(), "Calling listen"};
        }

        felspar::coro::starter<> co;
        for (auto acceptor = felspar::io::accept(ward, fd);
             auto cnx = co_await cancellation.signal_or(acceptor.next());) {
            co.post(echo_connection, ward, felspar::posix::fd{*cnx});
            co.garbage_collect_completed();
        }
        co_await co.wait_for_all();
    }
    felspar::coro::task<void> server_manager(
            felspar::io::warden &ward, felspar::posix::fd control) {
        felspar::coro::starter<> server;
        felspar::coro::cancellable cancellation;
        server.post(echo_server, std::ref(ward), std::ref(cancellation));
        std::cout << "Echo server running, waiting for end signal" << std::endl;
        std::array<std::byte, 8> buffer;
        co_await ward.read_some(control, buffer);
        std::cout << "Server has seen end signal, terminating" << std::endl;
        cancellation.cancel();
        co_await server.wait_for_all();
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
                    try {
                        felspar::io::uring_warden ward{1000};
                        ward.run(server_manager, std::move(control));
                    } catch (std::exception const &e) {
                        std::cerr << "Exception caught in server thread: "
                                  << e.what() << std::endl;
                        std::exit(2);
                    }
                },
                std::move(control_pipe.read)};

        co_await ward.sleep(100ms);
        std::cout << "Starting clients" << std::endl;
        felspar::coro::starter<> clients;
        while (clients.size() < 20) { clients.post(client, std::ref(ward)); }

        co_await ward.sleep(2s);
        std::cout << "Test done, signalling server to stop " << std::endl;
        std::array<std::byte, 1> signal{std::byte{'x'}};
        while (not co_await ward.write_some(control_pipe.write, signal))
            ;
        std::cout << "Waiting for server thread to end" << std::endl;
        co_await clients.wait_for_all();
        server.join();
        std::cout << "Server count started " << server_count_started
                  << " completed " << server_count_completed << '\n';
        co_return 0;
    }


}


int main() {
    try {
        felspar::io::uring_warden ward{1000};
        felspar::coro::starter<felspar::coro::task<void>> clients;
        return ward.run(co_main);
    } catch (std::exception const &e) {
        std::cerr << "Exception caught: " << e.what() << '\n';
        return 1;
    }
}
