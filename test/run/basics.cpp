#include <felspar/poll.hpp>
#include <felspar/test.hpp>


namespace {


    auto const suite = felspar::testsuite("basics");


    felspar::coro::task<void>
            echo_server(felspar::poll::executor &exec, std::uint16_t port) {
        co_return;
    }

    auto const trans = suite.test("transfer", []() {
        felspar::poll::executor exec;
        exec.post(echo_server, 5543);
    });


}
