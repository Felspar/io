#include <felspar/io.hpp>
#include <felspar/test.hpp>


using namespace std::literals;


namespace {


    auto const suite = felspar::testsuite("timers");


    felspar::coro::task<bool> short_sleep(felspar::io::warden &ward) {
        auto const start = std::chrono::steady_clock::now();
        co_await ward.sleep(20ms);
        auto const slept = std::chrono::steady_clock::now() - start;
        co_return slept >= 20ms and slept <= 30ms;
    }
    auto const ss = suite.test("timers/poll", [](auto check) {
                             felspar::io::poll_warden ward;
                             check(ward.run(short_sleep)) == true;
                         }).test("timers/io_uring", [](auto check) {
        felspar::io::io_uring_warden ward{5};
        check(ward.run(short_sleep)) == true;
    });


}
