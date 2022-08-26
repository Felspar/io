#include <felspar/coro/start.hpp>
#include <felspar/io.hpp>
#include <felspar/memory/hexdump.hpp>

#include <iostream>


namespace {

    /**
     * ## HTTP requests
     *
     * Processes HTTP requests on the connection.
     */
    felspar::coro::task<void>
            http_request(felspar::io::warden &ward, felspar::posix::fd fd) {
        std::cout << "Got HTTP request\n";
        felspar::io::read_buffer<std::array<char, 2 << 10>> buffer;
        auto const request{
                co_await felspar::io::read_until_lf_strip_cr(ward, fd, buffer)};
        std::cout << "Request: " << felspar::memory::hexdump(request);
        for (auto header = co_await felspar::io::read_until_lf_strip_cr(
                     ward, fd, buffer);
             header.size();
             header = co_await felspar::io::read_until_lf_strip_cr(
                     ward, fd, buffer)) {
            std::cout << felspar::memory::hexdump(header);
        }
        /// TODO Send response
        co_await write_all(
                ward, fd,
                std::string_view{"HTTP/1.0 200 OK\r\n"
                                 "Content-Length: 3\r\n"
                                 "\r\n"
                                 "OK\n"});
        co_await ward.close(std::move(fd));
        std::cout << "And we're done\n";
        co_return;
    }

    /**
     * ## Accept loop
     *
     * There will be one of these per thread.
     */
    felspar::coro::task<void> accept_loop(
            felspar::io::warden &ward, const felspar::posix::fd &socket) {
        felspar::coro::starter<felspar::coro::task<void>> co;
        for (auto connections = felspar::io::accept(ward, socket);
             auto cnx = co_await connections.next();) {
            co.post(http_request, std::ref(ward), felspar::posix::fd{*cnx});
            co.garbage_collect_completed();
        }
    }


    /**
     * ## Main
     */
    felspar::coro::task<int> co_main(felspar::io::warden &ward) {
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
