#include <felspar/io/deadline.hpp>
#include <felspar/test.hpp>


using namespace std::literals;


namespace {


    auto const suite = felspar::testsuite("deadline_from");


    auto const dt = suite.test("timeout", [](auto check) {
        /**
         * The deadline is computed from a `now()` taken inside `deadline_from`
         * which is at or after the `before` we capture here, so we allow the
         * same kind of jitter window the `timers` tests use.
         */
        auto const before = std::chrono::steady_clock::now();
        auto const d = felspar::io::deadline_from(20ms);
        check(d >= before + 19ms and d <= before + 21ms) == true;
    });


}
