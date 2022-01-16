#include <felspar/poll/accept.hpp>
#include <felspar/poll/read.hpp>
#include <felspar/poll/warden.hpp>
#include <felspar/poll/write.hpp>

#include <felspar/exceptions.hpp>


felspar::coro::stream<int> felspar::poll::accept(warden &ward, int fd) {
    while (true) {
        int s = co_await ward.accept(fd);
        if (s >= 0) {
            co_yield s;
        } else if (s == -11) {
            throw felspar::stdexcept::system_error{
                    -s, std::generic_category(), "accept"};
        } else {
            co_return;
        }
    }
}
