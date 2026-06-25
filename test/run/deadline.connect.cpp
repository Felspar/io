#include <felspar/io.hpp>
#include <felspar/test.hpp>


using namespace std::literals;


namespace {


    auto const suite = felspar::testsuite("deadline/connect");


    /**
     * `connect`-by-hostname must bound its connection attempt by the deadline.
     * We connect to a single RFC 5737 TEST-NET-1 example address, which is
     * never routed, so the attempt hangs until the deadline rather than being
     * refused outright, and we expect a timeout about one deadline after
     * starting.
     */
    felspar::io::warden::task<void> hostname_connect_deadline(
            felspar::io::warden &ward, char const *const hostname) {
        felspar::test::injected check;

        auto const begin = std::chrono::steady_clock::now();
        try {
            co_await felspar::io::connect(ward, hostname, 808, begin + 250ms);
            check(false) == true;
        } catch (felspar::io::timeout const &) {
            check(true) == true;
        } catch (std::exception const &e) {
            check.failed(e.what());
        } catch (...) { check(false) == true; }
        auto const elapsed = std::chrono::steady_clock::now() - begin;
        /**
         * The connect must be bounded by the deadline rather than hanging
         * indefinitely
         */
        check(elapsed <= 400ms) == true;
    }
    auto const p = suite.test("poll", []() {
        felspar::io::poll_warden ward;
        ward.run(hostname_connect_deadline, "192.0.2.1");
    });
#ifdef FELSPAR_ENABLE_IO_URING
    auto const u = suite.test("uring", []() {
        felspar::io::uring_warden ward{5};
        ward.run(hostname_connect_deadline, "192.0.2.1");
    });
#endif


}
