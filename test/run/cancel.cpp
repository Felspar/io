#include <felspar/io.hpp>
#include <felspar/test.hpp>


using namespace std::literals;


namespace {


    auto const suite = felspar::testsuite("cancellation");


    felspar::io::warden::task<void> sleeper(felspar::io::warden &ward) {
        co_await ward.sleep(30ms);
    }
    felspar::io::warden::task<void> starter(felspar::io::warden &ward) {
        {
            felspar::io::warden::starter<void> start;
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
