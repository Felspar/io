#include <felspar/io.hpp>
#include <felspar/memory/hexdump.hpp>

#include <fstream>
#include <iostream>


namespace {


    /// ## HTTP response options
    std::span<std::byte const> short_text();
    std::span<std::byte const> big_octets();


    /**
     * ## HTTP requests
     *
     * Processes HTTP requests on the connection.
     */
    felspar::io::warden::task<void> http_request(
            felspar::io::warden &ward,
            felspar::posix::fd fd,
            std::span<std::byte const> const response) {
        felspar::io::read_buffer<std::array<char, 2 << 10>> buffer;
        [[maybe_unused]] auto const request{
                co_await felspar::io::read_until_lf_strip_cr(ward, fd, buffer)};
        for (auto header = co_await felspar::io::read_until_lf_strip_cr(
                     ward, fd, buffer);
             header.size();
             header = co_await felspar::io::read_until_lf_strip_cr(
                     ward, fd, buffer)) {
            // TODO The headers should really be processed
        }
        co_await write_all(ward, fd, response);
        co_await ward.close(std::move(fd));
        co_return;
    }

    /**
     * ## Accept loop
     *
     * There will be one of these per thread.
     */
    felspar::io::warden::task<void> accept_loop(
            felspar::io::warden &ward, const felspar::posix::fd &socket) {
        felspar::io::warden::starter<void> co;
        for (auto connections = felspar::io::accept(ward, socket);
             auto cnx = co_await connections.next();) {
            co.post(http_request, std::ref(ward), felspar::posix::fd{*cnx},
                    big_octets());
            co.garbage_collect_completed();
        }
    }


    /// ## Main
    felspar::io::warden::task<int> co_main(felspar::io::warden &ward) {
        constexpr std::uint16_t port{4040};
        constexpr int backlog = 64;

        /// Create the accept socket
        auto fd = ward.create_socket(AF_INET, SOCK_STREAM, 0);
        felspar::posix::set_reuse_port(fd);
        felspar::posix::bind_to_any_address(fd, port);
        felspar::posix::listen(fd, backlog);

        /// Run the web server forever
        co_await ::accept_loop(ward, fd);

        co_return 0;
    }


}


int main() {
    try {
        std::cout << "Starting web server for current directory\n";
        felspar::posix::promise_to_never_use_select();
        felspar::io::poll_warden ward{};
        return ward.run(co_main);
    } catch (std::exception const &e) {
        std::cerr << "Caught an exception: " << e.what() << '\n';
        return -1;
    }
}


namespace {


    [[maybe_unused]] std::span<std::byte const> short_text() {
        constexpr std::string_view sv{
                "HTTP/1.0 200 OK\r\n"
                "Content-Length: 3\r\n"
                "\r\n"
                "OK\n"};
        return {reinterpret_cast<std::byte const *>(sv.data()), sv.size()};
    }

    constexpr std::string_view prefix{
            "HTTP/1.0 200 OK\r\n"
            "Content-Length: "};
    auto const big{[]() {
        std::string buffer{prefix};
        constexpr std::size_t bytes{10 << 10};
        buffer += std::to_string(bytes);
        buffer += "\r\n\r\n";
        std::vector<char> data(bytes);
        std::ifstream{"/dev/urandom"}.read(data.data(), bytes);
        buffer += std::string_view{data.data(), bytes};
        return buffer;
    }()};
    std::span<std::byte const> big_octets() {
        return {reinterpret_cast<std::byte const *>(big.data()), big.size()};
    }

}
