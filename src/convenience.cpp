#include <felspar/io/accept.hpp>
#include <felspar/io/read.hpp>
#include <felspar/io/warden.hpp>
#include <felspar/io/write.hpp>

#include <felspar/exceptions.hpp>


felspar::coro::stream<felspar::io::socket_descriptor> felspar::io::accept(
        warden &ward, socket_descriptor fd, felspar::source_location loc) {
    while (true) {
        auto s = co_await ward.accept(fd, {}, loc);
        if (s >= 0) {
            co_yield s;
        } else if (s == -11) {
            throw felspar::stdexcept::system_error{
                    -s, std::system_category(), "accept", loc};
        } else {
            co_return;
        }
    }
}
