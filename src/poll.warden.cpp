#include "poll.hpp"

#include <felspar/exceptions.hpp>
#include <felspar/io/posix.hpp>

#include <poll.h>


void felspar::io::poll_warden::run_until(felspar::coro::coroutine_handle<> coro) {
    coro.resume();
    std::vector<::pollfd> iops;
    std::vector<retrier *> continuations;
    while (not coro.done()) {
        /// IOP loop
        iops.clear();
        for (auto const &req : requests) {
            short flags = {};
            if (not req.second.reads.empty()) { flags |= POLLIN; }
            if (not req.second.writes.empty()) { flags |= POLLOUT; }
            iops.push_back({req.first, flags, {}});
        }

        int pr;
        if (timeouts.begin() != timeouts.end()) {
            pr = ::poll(
                    iops.data(), iops.size(),
                    std::max(
                            std::chrono::milliseconds{},
                            std::chrono::duration_cast<std::chrono::milliseconds>(
                                    timeouts.begin()->first
                                    - std::chrono::steady_clock::now()))
                            .count());
        } else {
            pr = ::poll(iops.data(), iops.size(), -1);
        }
        if (pr == -1) {
            throw felspar::stdexcept::system_error{
                    errno, std::generic_category(), "poll"};
        } else if (pr == 0) {
            /// Process time outs
            while (timeouts.begin() != timeouts.end()) {
                auto const tdiff = timeouts.begin()->first
                        - std::chrono::steady_clock::now();
                if (tdiff < std::chrono::milliseconds{1}) {
                    timeouts.begin()->second->iop_timedout();
                    timeouts.erase(timeouts.begin());
                } else {
                    break;
                }
            }
        } else {
            /// Continuations loop
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
                continuation->try_or_resume();
            }
        }
    }
}


felspar::posix::fd felspar::io::poll_warden::create_socket(
        int domain, int type, int protocol, felspar::source_location loc) {
    auto fd = warden::create_socket(domain, type, protocol, loc);
    felspar::posix::set_non_blocking(fd);
    return fd;
}
