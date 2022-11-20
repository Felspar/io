#include "poll.hpp"

#include <felspar/exceptions.hpp>
#include <felspar/io/posix.hpp>

#if __has_include(<poll.h>)
#include <poll.h>
#endif


void felspar::io::poll_warden::run_until(felspar::coro::coroutine_handle<> coro) {
    coro.resume();
#if defined(FELSPAR_WINSOCK2)
    std::vector<::WSAPOLLFD> iops;
#else
    std::vector<::pollfd> iops;
#endif
    std::vector<retrier *> continuations;

    auto const clear_timeouts = [&]() -> int {
        while (timeouts.begin() != timeouts.end()) {
            auto const tdiff =
                    timeouts.begin()->first - std::chrono::steady_clock::now();
            if (tdiff < std::chrono::milliseconds{1}) {
                retrier *const retry = timeouts.begin()->second;
                timeouts.erase(timeouts.begin());
                retry->iop_timedout().resume();
            } else {
                return std::chrono::duration_cast<std::chrono::milliseconds>(
                               tdiff)
                        .count();
            }
        }
        return -1;
    };

    while (not coro.done()) {
        auto const timeout = clear_timeouts();

        iops.clear();
        for (auto const &req : requests) {
            short flags = {};
            if (not req.second.reads.empty()) { flags |= POLLIN; }
            if (not req.second.writes.empty()) { flags |= POLLOUT; }
            iops.push_back({req.first, flags, {}});
        }

        int const pr = [&]() {
            if (iops.size()) {
#if defined(FELSPAR_WINSOCK2)
                return ::WSAPoll(iops.data(), iops.size(), timeout);
#else
                return ::poll(iops.data(), iops.size(), timeout);
#endif
            } else {
                ::timespec req{{}, timeout * 1'000'000}, rem{};
                while (::nanosleep(&req, &rem) == -1 and errno == EINTR) {
                    req = rem;
                }
                return 0;
            }
        }();
        if (pr < 0) {
#if defined(FELSPAR_WINSOCK2)
            throw felspar::stdexcept::system_error{
                    WSAGetLastError(), std::system_category(), "WSAPoll"};
#else
            throw felspar::stdexcept::system_error{
                    errno, std::system_category(), "poll"};
#endif
        } else if (pr > 0) {
            continuations.clear();
            for (auto events : iops) {
                if (events.revents & POLLIN) {
                    auto &reads = requests[events.fd].reads;
                    continuations.insert(
                            continuations.end(), reads.begin(), reads.end());
                    reads.clear();
                }
                if (events.revents & POLLOUT) {
                    auto &writes = requests[events.fd].writes;
                    continuations.insert(
                            continuations.end(), writes.begin(), writes.end());
                    writes.clear();
                }
            }
            for (auto continuation : continuations) {
                continuation->try_or_resume().resume();
            }
        }
    }
}


felspar::posix::fd felspar::io::poll_warden::do_create_socket(
        int domain,
        int type,
        int protocol,
        felspar::source_location const &loc) {
    auto fd = warden::do_create_socket(domain, type, protocol, loc);
    felspar::posix::set_non_blocking(fd);
    return fd;
}
