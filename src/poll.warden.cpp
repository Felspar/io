#include "poll.hpp"

#include <felspar/exceptions.hpp>
#include <felspar/io/posix.hpp>

#include <thread>

#if __has_include(<poll.h>)
#include <poll.h>
#endif
#if not defined(FELSPAR_WINSOCK2)
#include <signal.h>
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
#else
    ::signal(SIGPIPE, SIG_IGN);
#endif
}


felspar::io::poll_warden::~poll_warden() {
#if defined(FELSPAR_WINSOCK2)
    WSACleanup();
#endif
}


void felspar::io::poll_warden::run_until(std::coroutine_handle<> coro) {
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
            int response = ::WSAPoll(
                    bookkeeping->iops.data(),
                    static_cast<ULONG>(bookkeeping->iops.size()), timeout);
            if (response < 0) {
                /**
                 * Windows seems to return errors here for no good reason, but
                 * even if it is for a good reason, we kind of need to keep
                 * going anyway (for example if the network is down). We'll let
                 * things time out and have the application's error handling
                 * take over instead.
                 *
                 * We'll just pretend that one of the file descriptors was
                 * signalled (the number isn't ever used so it's value doesn't
                 * matter).
                 */
                return 1;
            } else {
                return response;
            }
#else
            return ::poll(
                    bookkeeping->iops.data(), bookkeeping->iops.size(),
                    timeout);
#endif
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds{timeout});
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
        socket_descriptor sock, std::source_location const &loc) {
    felspar::posix::set_non_blocking(sock, loc);
}
