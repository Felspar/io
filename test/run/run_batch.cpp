#include <felspar/io.hpp>
#include <felspar/test.hpp>
#include <felspar/coro/eager.hpp>


using namespace std::literals;


namespace {


    auto const suite = felspar::testsuite("run_batch");


    felspar::io::warden::task<void>
            accept_forever(felspar::io::warden &ward, std::uint16_t port) {
        auto fd = ward.create_socket(AF_INET, SOCK_STREAM, 0);
        set_reuse_port(fd);
        bind_to_any_address(fd, port);
        listen(fd, 64);
        co_await ward.accept(fd);
    }
    auto const ap = suite.test("accept/poll", [](auto check) {
        felspar::io::poll_warden ward;
        felspar::io::warden::eager<> accept;
        accept.post(accept_forever, std::ref(ward), 5544);
        auto const start = std::chrono::steady_clock::now();
        ward.run_batch();
        auto const taken = std::chrono::steady_clock::now() - start;
        check(taken) < 15ms;
    });
#ifdef FELSPAR_ENABLE_IO_URING
    auto const au = suite.test("accept/io_uring", [](auto check, auto &log) {
        felspar::io::uring_warden ward;
        felspar::io::warden::eager<> accept;
        accept.post(accept_forever, std::ref(ward), 5546);
        auto const start = std::chrono::steady_clock::now();
        ward.run_batch();
        auto const taken = std::chrono::steady_clock::now() - start;
        check(taken) < 5ms;
    });
#endif


}
