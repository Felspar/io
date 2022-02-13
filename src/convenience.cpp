#include <felspar/io/accept.hpp>
#include <felspar/io/read.hpp>
#include <felspar/io/warden.hpp>
#include <felspar/io/write.hpp>

#include <felspar/exceptions.hpp>


felspar::coro::stream<int> felspar::io::accept(
        warden &ward, int fd, felspar::source_location loc) {
    while (true) {
        int s = co_await ward.accept(fd, {}, loc);
        if (s >= 0) {
            co_yield s;
        } else if (s == -11) {
            throw felspar::stdexcept::system_error{
                    -s, std::generic_category(), "accept", std::move(loc)};
        } else {
            co_return;
        }
    }
}
