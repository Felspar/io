#include <felspar/io/pipe.hpp>
#include <felspar/test.hpp>

#include <felspar/io/read.hpp>
#include <felspar/io/warden.poll.hpp>
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


    auto const tp = suite.test("transmission", []() {
        felspar::io::poll_warden ward;
        ward.run(
                +[](felspar::io::warden &ward)
                        -> felspar::io::warden::task<void> {
                    felspar::test::injected check;

                    auto pipe = ward.create_pipe();

                    std::array<std::uint8_t, 6> out{1, 2, 3, 4, 5, 6}, buffer{};
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
    }, []() {
        felspar::io::poll_warden ward;
        ward.run(
                +[](felspar::io::warden &ward)
                        -> felspar::io::warden::task<void> {
                    felspar::test::injected check;

                    auto pipe = ward.create_pipe();

                    std::array<std::uint8_t, 6> out{1, 2, 3, 4, 5, 6}, buffer{};
                    check(felspar::io::write_some(pipe.write.native_handle(), out.data(), out.size())) == out.size();

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


}
