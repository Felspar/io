#include <felspar/io.hpp>
#include <felspar/test.hpp>
#include <felspar/coro/eager.hpp>

#include <thread>


using namespace std::literals;


namespace {


    auto const suite = felspar::testsuite("run_batch");


    felspar::io::warden::task<void> accept_forever(
            felspar::io::warden &ward, std::uint16_t const port) {
        auto fd = ward.create_socket(AF_INET, SOCK_STREAM, 0);
        set_reuse_port(fd);
        bind_to_any_address(fd, port);
        listen(fd, 64);
        co_await ward.accept(fd);
    }

    felspar::io::warden::task<void>
            do_connect(felspar::io::warden &ward, std::uint16_t const port) {
        auto fd = ward.create_socket(AF_INET, SOCK_STREAM, 0);

        sockaddr_in in;
        in.sin_family = AF_INET;
        in.sin_port = htons(port);
        in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        co_await ward.connect(
                fd, reinterpret_cast<sockaddr const *>(&in), sizeof(in));
    }

    auto time_loop(felspar::io::warden &ward) {
        auto const start = std::chrono::steady_clock::now();
        ward.run_batch();
        return std::chrono::steady_clock::now() - start;
    }


    auto const ap = suite.test("accept/poll", [](auto check) {
        felspar::io::poll_warden ward;

        felspar::io::warden::eager<> accept;
        accept.post(accept_forever, std::ref(ward), 5544);
        check(time_loop(ward)) < 15ms;

        felspar::io::warden::eager<> connect;
        connect.post(do_connect, std::ref(ward), 5544);
        check(time_loop(ward)) < 15ms;

#ifdef __APPLE__
        std::this_thread::sleep_for(10ms);
        check(time_loop(ward)) < 15ms;
#endif
        /// Should not hang
        std::move(accept).release().get();
    });
#ifdef FELSPAR_ENABLE_IO_URING
    auto const au = suite.test("accept/io_uring", [](auto check, auto &log) {
        felspar::io::uring_warden ward;

        felspar::io::warden::eager<> accept;
        accept.post(accept_forever, std::ref(ward), 5546);
        check(time_loop(ward)) < 5ms;

        felspar::io::warden::eager<> connect;
        connect.post(do_connect, std::ref(ward), 5546);
        check(time_loop(ward)) < 5ms;

        /// Should not hang
        accept.release().get();
    });
#endif


}
