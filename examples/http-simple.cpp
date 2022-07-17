#include <felspar/coro/start.hpp>
#include <felspar/io.hpp>

#include <iostream>


namespace {


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
            co.garbage_collect_completed();
        }
    }


    /**Calling POSIX
     * ## Main
     */
    felspar::coro::task<int> co_main(felspar::io::warden &ward) {
        constexpr std::uint16_t port{61789};
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
