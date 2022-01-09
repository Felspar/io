#include <felspar/poll/accept.hpp>
#include <felspar/poll/warden.hpp>


felspar::coro::stream<int> felspar::poll::accept(warden &ward, int fd) {
    while (true) {
        int s = co_await ward.accept(fd);
        if (s >= 0) {
            co_yield s;
        } else {
            co_return;
        }
    }
}
