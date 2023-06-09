#include <felspar/io/posix.hpp>
#include <felspar/test.hpp>


namespace {


    auto const suite = felspar::testsuite("pipe");


    auto const cons = suite.test("construct", [](auto check) {
        auto p = felspar::posix::pipe::create();
        check(p.read).is_truthy();
        check(p.write).is_truthy();
    });


}
