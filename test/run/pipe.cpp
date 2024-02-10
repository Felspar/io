#include <felspar/io/posix.hpp>
#include <felspar/test.hpp>

#include <felspar/io/read.hpp>
#include <felspar/io/warden.poll.hpp>
#include <felspar/io/write.hpp>


using namespace std::literals;


namespace {


    auto const suite = felspar::testsuite("pipe");


    auto const cons = suite.test("construct", [](auto check) {
        auto p = felspar::posix::pipe::create();
        check(p.read).is_truthy();
        check(p.write).is_truthy();
    });


    auto const tp = suite.test("transmission", []() {
        felspar::io::poll_warden ward;
        auto pipe = felspar::posix::pipe::create();
        ward.run(
                +[](felspar::io::warden &ward, felspar::posix::pipe &pipe)
                        -> felspar::io::warden::task<void> {
                    felspar::test::injected check;

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
                },
                std::ref(pipe));
    });


}
