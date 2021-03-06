#include <felspar/coro/start.hpp>
#include <felspar/io.hpp>
#include <felspar/test.hpp>


using namespace std::literals;


namespace {


    auto const suite = felspar::testsuite("cancellation");


    felspar::coro::task<void> sleeper(felspar::io::warden &ward) {
        co_await ward.sleep(30ms);
    }
    felspar::coro::task<void> starter(felspar::io::warden &ward) {
        {
            felspar::coro::starter<felspar::coro::task<void>> start;
            start.post(sleeper, std::ref(ward));
        }
        co_await ward.sleep(10ms);
    }
    auto const pc = suite.test("poll/cancel", []() {
        felspar::io::poll_warden ward;
        ward.run(starter);
    });
#ifdef FELSPAR_ENABLE_IO_URING
    auto const uc = suite.test("io_uring/cancel", []() {
        felspar::io::uring_warden ward{5};
        ward.run(starter);
    });
#endif


}
