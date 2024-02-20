#include "poll.hpp"

#include <felspar/exceptions.hpp>
#include <felspar/io/posix.hpp>

#if __has_include(<poll.h>)
#include <poll.h>
#endif


struct felspar::io::poll_warden::loop_data {
#if defined(FELSPAR_WINSOCK2)
    std::vector<::WSAPOLLFD> iops;
#else
    std::vector<::pollfd> iops;
#endif
    std::vector<retrier *> continuations;
};


felspar::io::poll_warden::poll_warden()
: bookkeeping{std::make_unique<loop_data>()} {
#if defined(FELSPAR_WINSOCK2)
    WORD vreq = MAKEWORD(2, 0);
    WSADATA sadat;
    WSAStartup(vreq, &sadat);
#endif
}


felspar::io::poll_warden::~poll_warden() {
#if defined(FELSPAR_WINSOCK2)
    WSACleanup();
#endif
}


void felspar::io::poll_warden::run_until(felspar::coro::coroutine_handle<> coro) {
    coro.resume();
    while (true) {
        auto const timeout = clear_timeouts();
        if (coro.done()) { return; }
        do_poll(timeout);
    }
}


void felspar::io::poll_warden::run_batch() {
    clear_timeouts();
    do_poll(0);
}


void felspar::io::poll_warden::do_poll(int const timeout) {
    bookkeeping->iops.clear();
    for (auto const &req : requests) {
        short flags = {};
        if (not req.second.reads.empty()) { flags |= POLLIN; }
        if (not req.second.writes.empty()) { flags |= POLLOUT; }
        bookkeeping->iops.push_back({req.first, flags, {}});
#if defined(FELSPAR_WINSOCK2)
        if (bookkeeping->iops.size() == std::numeric_limits<ULONG>::max()) {
            break;
        }
#endif
    }

    int const pr = [&]() {
        if (bookkeeping->iops.size()) {
#if defined(FELSPAR_WINSOCK2)
            return ::WSAPoll(
                    bookkeeping->iops.data(),
                    static_cast<ULONG>(bookkeeping->iops.size()), timeout);
#else
            return ::poll(
                    bookkeeping->iops.data(), bookkeeping->iops.size(),
                    timeout);
#endif
        } else {
#if defined(FELSPAR_WINSOCK2)
            ::Sleep(timeout);
#else
            ::timespec req{{}, timeout * 1'000'000}, rem{};
            while (::nanosleep(&req, &rem) == -1 and errno == EINTR) {
                req = rem;
            }
#endif
            return 0;
        }
    }();
    if (pr < 0) {
        throw felspar::stdexcept::system_error{
                get_error(), std::system_category(), "poll"};
    } else if (pr > 0) {
        bookkeeping->continuations.clear();
        for (auto events : bookkeeping->iops) {
            if (events.revents & (POLLIN | POLLERR | POLLNVAL)) {
                auto &reads = requests[events.fd].reads;
                bookkeeping->continuations.insert(
                        bookkeeping->continuations.end(), reads.begin(),
                        reads.end());
                reads.clear();
            }
            if (events.revents & (POLLOUT | POLLERR | POLLNVAL)) {
                auto &writes = requests[events.fd].writes;
                bookkeeping->continuations.insert(
                        bookkeeping->continuations.end(), writes.begin(),
                        writes.end());
                writes.clear();
            }
        }
        for (auto continuation : bookkeeping->continuations) {
            continuation->try_or_resume().resume();
        }
    }
}


int felspar::io::poll_warden::clear_timeouts() {
    while (timeouts.begin() != timeouts.end()) {
        auto const tdiff =
                timeouts.begin()->first - std::chrono::steady_clock::now();
        if (tdiff < std::chrono::milliseconds{1}) {
            retrier *const retry = timeouts.begin()->second;
            timeouts.erase(timeouts.begin());
            retry->iop_timedout().resume();
        } else {
            return std::chrono::duration_cast<std::chrono::milliseconds>(tdiff)
                    .count();
        }
    }
    return -1;
}


void felspar::io::poll_warden::do_prepare_socket(
        socket_descriptor sock, felspar::source_location const &loc) {
    felspar::posix::set_non_blocking(sock, loc);
}
