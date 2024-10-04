#include <felspar/io.hpp>
#include <felspar/test.hpp>


using namespace std::literals;


namespace {


    auto const suite = felspar::testsuite("warden::run exceptions");


    void always_throw() {
        throw felspar::stdexcept::runtime_error{"Forced exception"};
    }


    auto const wrep = suite.test("early poll", [](auto check) {
        check([&]() {
            felspar::io::poll_warden ward;
            ward.run(
                    +[](felspar::io::warden &)
                            -> felspar::io::warden::task<void> {
                        always_throw();
                        co_return;
                    });
        }).template throws_type<felspar::stdexcept::runtime_error>();
    });


    auto const wrlp = suite.test("late poll", [](auto check) {
        check([&]() {
            felspar::io::poll_warden ward;
            ward.run(
                    +[](felspar::io::warden &ward)
                            -> felspar::io::warden::task<void> {
                        co_await ward.sleep(10ms);
                        always_throw();
                    });
        }).template throws_type<felspar::stdexcept::runtime_error>();
    });


#ifdef FELSPAR_ENABLE_IO_URING
    auto const wreu = suite.test("early io_uring", [](auto check) {
        check([&]() {
            felspar::io::uring_warden ward;
            ward.run(
                    +[](felspar::io::warden &)
                            -> felspar::io::warden::task<void> {
                        always_throw();
                        co_return;
                    });
        }).template throws_type<felspar::stdexcept::runtime_error>();
    });


    auto const wrlu = suite.test("late io_uring", [](auto check) {
        check([&]() {
            felspar::io::uring_warden ward;
            ward.run(
                    +[](felspar::io::warden &ward)
                            -> felspar::io::warden::task<void> {
                        co_await ward.sleep(10ms);
                        always_throw();
                    });
        }).template throws_type<felspar::stdexcept::runtime_error>();
    });
#endif


}
