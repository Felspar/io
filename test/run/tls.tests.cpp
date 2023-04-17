#include <felspar/io/read.hpp>
#include <felspar/io/tls.hpp>
#include <felspar/io/warden.poll.hpp>
#include <felspar/io/write.hpp>
#include <felspar/test.hpp>

#include <sys/types.h>
#if defined(FELSPAR_POSIX_SOCKETS)
#include <sys/socket.h>
#include <netdb.h>
#endif


using namespace std::literals;


namespace {


    auto const suite = felspar::testsuite("tls");


    felspar::io::warden::task<void> test_connect(
            felspar::io::warden &warden,
            char const *const hostname,
            felspar::test::injected check) {
        struct addrinfo hints = {};
        hints.ai_socktype = SOCK_STREAM;
        struct addrinfo *addresses = nullptr;
        check(getaddrinfo(hostname, nullptr, &hints, &addresses)) == 0;
        check(addresses) != nullptr;

        sockaddr_in address =
                *reinterpret_cast<sockaddr_in *>(addresses->ai_addr);
        address.sin_port = htons(443);
        freeaddrinfo(addresses);

        auto website = co_await felspar::io::tls::connect(
                warden, hostname, reinterpret_cast<sockaddr const *>(&address),
                sizeof(address), 5s);

        constexpr std::string_view request = "GET / HTTP/1.0\r\n\r\n";

        auto written =
                co_await felspar::io::write_all(warden, website, request);
        check(written) == request.size();

        felspar::io::read_buffer<std::array<char, 2 << 10>> buffer;
        auto line1 = co_await felspar::io::read_until_lf_strip_cr(
                warden, website, buffer);

        constexpr std::string_view expected = "HTTP/1.1 200 OK";
        check(line1.size()) == expected.size();
        check(std::equal(
                line1.begin(), line1.end(), expected.begin(), expected.end()))
                == true;
    }


    auto const connect = suite.test("connect", [](auto check) {
        felspar::io::poll_warden ward;
        ward.run(test_connect, "felspar.com", check);
    });


}
