#include <felspar/io.hpp>
#include <felspar/test.hpp>


using namespace std::literals;


namespace {


    auto const suite = felspar::testsuite("deadline/connect");


    /**
     * `connect`-by-hostname tries each address the hostname resolves to in
     * turn. A single deadline must bound the whole attempt loop, so even though
     * the host resolves to several addresses that all silently drop the
     * connection we still time out about one deadline after starting rather
     * than resetting the timeout for every address.
     */
    felspar::io::warden::task<void> hostname_connect_deadline(
            felspar::io::warden &ward, char const *const hostname) {
        felspar::test::injected check;

        auto const begin = std::chrono::steady_clock::now();
        try {
            /**
             * Port 808 is silently dropped by the host so the connect hangs
             * until the deadline rather than being refused outright
             */
            co_await felspar::io::connect(ward, hostname, 808, begin + 250ms);
            check(false) == true;
        } catch (felspar::io::timeout const &) {
            check(true) == true;
        } catch (std::exception const &e) {
            check.failed(e.what());
        } catch (...) { check(false) == true; }
        auto const elapsed = std::chrono::steady_clock::now() - begin;
        /**
         * With a per-address timeout the multiple addresses would push this
         * well past 2x the deadline; the single shared deadline keeps the whole
         * operation bounded by roughly one deadline
         */
        check(elapsed <= 400ms) == true;
    }
    auto const p = suite.test("poll", []() {
        felspar::io::poll_warden ward;
        ward.run(hostname_connect_deadline, "felspar.com");
    });
#ifdef FELSPAR_ENABLE_IO_URING
    auto const u = suite.test("uring", []() {
        felspar::io::uring_warden ward{5};
        ward.run(hostname_connect_deadline, "felspar.com");
    });
#endif


}
