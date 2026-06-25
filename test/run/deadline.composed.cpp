#include <felspar/io.hpp>
#include <felspar/test.hpp>


using namespace std::literals;


namespace {


    auto const suite = felspar::testsuite("deadline/composed");


    /**
     * Drip non-LF bytes one at a time so a composed read is forced to make many
     * partial `read_some` calls before its deadline is reached. The stream
     * never ends and never contains a newline.
     */
    felspar::io::warden::task<void> drip_writer(
            felspar::io::warden &ward, felspar::posix::fd &write_end) {
        std::array<std::byte, 1> byte{std::byte{'x'}};
        while (true) {
            co_await ward.sleep(5ms);
            co_await felspar::io::write_all(
                    ward, write_end, std::span<std::byte const>{byte});
        }
    }


    /**
     * `read_exactly` must time out about one deadline after it starts even
     * though bytes keep arriving -- the single deadline bounds the whole
     * operation, not each `read_some`.
     */
    felspar::io::warden::task<void>
            read_exactly_deadline(felspar::io::warden &ward) {
        felspar::test::injected check;

        auto pipe = ward.create_pipe();
        felspar::io::warden::starter<void> start;
        start.post(drip_writer, std::ref(ward), std::ref(pipe.write));

        std::array<std::byte, 128> buffer{};
        auto const begin = std::chrono::steady_clock::now();
        try {
            co_await felspar::io::read_exactly(ward, pipe.read, buffer, 50ms);
            check(false) == true;
        } catch (felspar::io::timeout const &) {
            check(true) == true;
        } catch (...) { check(false) == true; }
        auto const elapsed = std::chrono::steady_clock::now() - begin;
        check(elapsed <= 300ms) == true;
    }
    auto const rep = suite.test("read-exactly-deadline/poll", []() {
        felspar::io::poll_warden ward;
        ward.run(read_exactly_deadline);
    });
#ifdef FELSPAR_ENABLE_IO_URING
    auto const reu = suite.test("read-exactly-deadline/io_uring", []() {
        felspar::io::uring_warden ward{5};
        ward.run(read_exactly_deadline);
    });
#endif


    /**
     * `read_until_lf_strip_cr` never sees a newline so it keeps issuing
     * `do_read_some` calls; the single deadline must still bound the whole line
     * read across all of those partial reads.
     */
    felspar::io::warden::task<void>
            read_until_deadline(felspar::io::warden &ward) {
        felspar::test::injected check;

        auto pipe = ward.create_pipe();
        felspar::io::warden::starter<void> start;
        start.post(drip_writer, std::ref(ward), std::ref(pipe.write));

        felspar::io::read_buffer<std::array<char, 1 << 10>> buffer;
        auto const begin = std::chrono::steady_clock::now();
        try {
            co_await felspar::io::read_until_lf_strip_cr(
                    ward, pipe.read, buffer, 50ms);
            check(false) == true;
        } catch (felspar::io::timeout const &) {
            check(true) == true;
        } catch (...) { check(false) == true; }
        auto const elapsed = std::chrono::steady_clock::now() - begin;
        check(elapsed <= 300ms) == true;
    }
    auto const rup = suite.test("read-until-deadline/poll", []() {
        felspar::io::poll_warden ward;
        ward.run(read_until_deadline);
    });
#ifdef FELSPAR_ENABLE_IO_URING
    auto const ruu = suite.test("read-until-deadline/io_uring", []() {
        felspar::io::uring_warden ward{5};
        ward.run(read_until_deadline);
    });
#endif


}
