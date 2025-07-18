#include <felspar/io/addrinfo.hpp>
#include <felspar/io/read.hpp>
#include <felspar/io/tls.hpp>
#include <felspar/io/warden.poll.hpp>
#include <felspar/io/write.hpp>
#include <felspar/memory/hexdump.hpp>
#include <felspar/test.hpp>


using namespace std::literals;


namespace {


    auto const suite = felspar::testsuite("tls");


    felspar::io::warden::task<void> test_connect(
            felspar::io::warden &warden,
            char const *const hostname,
            felspar::test::injected check,
            std::ostream &log) {

        auto addresses = felspar::io::addrinfo(hostname, 443);
        auto address = *addresses.next();

        auto website = co_await felspar::io::tls::connect(
                warden, hostname, address.first, address.second, 5s);

        auto const request =
                std::string{"GET / HTTP/1.0\r\nHost: "} + hostname + "\r\n\r\n";

        auto written =
                co_await felspar::io::write_all(warden, website, request);
        check(written) == request.size();

        felspar::io::read_buffer<std::array<char, 2 << 10>> buffer;
        auto line1 = co_await felspar::io::read_until_lf_strip_cr(
                warden, website, buffer);

        constexpr std::string_view expected = "HTTP/1.1 200 OK";
        log << felspar::memory::hexdump(line1);
        check(line1.size()) == expected.size();
        check(std::equal(
                line1.begin(), line1.end(), expected.begin(), expected.end()))
                == true;
    }


    auto const connect = suite.test("connect", [](auto check, auto &log) {
        felspar::io::poll_warden ward;
        ward.run(test_connect, "felspar.com", check, std::ref(log));
    });


}
