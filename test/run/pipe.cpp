#include <felspar/io/pipe.hpp>
#include <felspar/test.hpp>

#include <felspar/io/read.hpp>
#include <felspar/io/warden.poll.hpp>
#ifdef FELSPAR_ENABLE_IO_URING
#include <felspar/io/warden.uring.hpp>
#endif
#include <felspar/io/write.hpp>


using namespace std::literals;


namespace {


    auto const suite = felspar::testsuite("pipe");


    auto const cons = suite.test("construct", [](auto check) {
        felspar::io::poll_warden ward;
        auto p = ward.create_pipe();
        check(p.read).is_truthy();
        check(p.write).is_truthy();
    });


    auto const tp = suite.test(
            "transmission",
            []() {
                felspar::io::poll_warden ward;
                ward.run(
                        +[](felspar::io::warden &ward)
                                -> felspar::io::warden::task<void> {
                            felspar::test::injected check;

                            auto pipe = ward.create_pipe();

                            std::array<std::uint8_t, 6> out{1, 2, 3, 4, 5, 6},
                                    buffer{};
                            co_await felspar::io::write_all(
                                    ward, pipe.write, out, 20ms);

                            auto bytes = co_await felspar::io::read_exactly(
                                    ward, pipe.read, buffer, 20ms);
                            check(bytes) == 6u;
                            check(buffer[0]) == out[0];
                            check(buffer[1]) == out[1];
                            check(buffer[2]) == out[2];
                            check(buffer[3]) == out[3];
                            check(buffer[4]) == out[4];
                            check(buffer[5]) == out[5];
                        });
            },
            []() {
                felspar::io::poll_warden ward;
                ward.run(
                        +[](felspar::io::warden &ward)
                                -> felspar::io::warden::task<void> {
                            felspar::test::injected check;

                            auto pipe = ward.create_pipe();

                            std::array<std::uint8_t, 6> out{1, 2, 3, 4, 5, 6},
                                    buffer{};
                            check(felspar::io::write_some(
                                    pipe.write.native_handle(), out.data(),
                                    out.size()))
                                    == out.size();

                            auto bytes = co_await felspar::io::read_exactly(
                                    ward, pipe.read, buffer, 20ms);
                            check(bytes) == 6u;
                            check(buffer[0]) == out[0];
                            check(buffer[1]) == out[1];
                            check(buffer[2]) == out[2];
                            check(buffer[3]) == out[3];
                            check(buffer[4]) == out[4];
                            check(buffer[5]) == out[5];
                        });
            });


    /// Record how many bytes a read returns, where zero signals EOF
    felspar::io::warden::task<void> record_read(
            felspar::io::warden &ward,
            felspar::posix::fd &read_end,
            std::span<std::uint8_t> const buffer,
            std::optional<std::size_t> &bytes_read) {
        bytes_read =
                co_await felspar::io::read_exactly(ward, read_end, buffer, 30s);
    }


    /// ### A read pending in the warden wakes when the write end hangs up
    /**
     * Closing the write end of a pipe makes the kernel report `POLLHUP` on the
     * read end without a separate readable event. A read that is already
     * suspended in the warden must wake promptly with EOF (zero bytes) rather
     * than sit blocked until its (here very long) timeout expires.
     */
    felspar::io::warden::task<void>
            hangup_wakes_pending_read(felspar::io::warden &ward) {
        felspar::test::injected check;

        auto pipe = ward.create_pipe();
        std::array<std::uint8_t, 6> buffer{};
        std::optional<std::size_t> bytes_read;
        {
            felspar::io::warden::starter<void> start;
            start.post(
                    record_read, std::ref(ward), std::ref(pipe.read),
                    std::span<std::uint8_t>{buffer}, std::ref(bytes_read));
            // Give the read time to register itself with the warden and suspend
            co_await ward.sleep(10ms);
            // Now hang up the write end -- the pending read must wake up
            pipe.write.close();
            co_await ward.sleep(10ms);
        }

        // If the hangup was missed the read would still be blocked here, so a
        // value being present is what proves it woke well within the timeout
        check(bytes_read.has_value()).is_truthy();
        check(bytes_read.value_or(1)) == 0u;
    }

    auto const hup = suite.test("hangup-wakes-read", []() {
        felspar::io::poll_warden ward;
        ward.run(hangup_wakes_pending_read);
    });
#ifdef FELSPAR_ENABLE_IO_URING
    auto const hup_uring = suite.test("hangup-wakes-read/io_uring", []() {
        felspar::io::uring_warden ward{5};
        ward.run(hangup_wakes_pending_read);
    });
#endif


}
