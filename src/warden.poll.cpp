#include <felspar/exceptions.hpp>
#include <felspar/poll/posix.hpp>
#include <felspar/poll/warden.poll.hpp>

#include <poll.h>


void felspar::poll::poll_warden::run_until(
        felspar::coro::unique_handle<felspar::coro::task_promise<void>> coro) {
    coro.resume();
    std::vector<::pollfd> iops;
    std::vector<completion *> continuations;
    while (not coro.done()) {
        /// IOP loop
        iops.clear();
        for (auto const &req : requests) {
            short flags = {};
            if (not req.second.reads.empty()) { flags |= POLLIN; }
            if (not req.second.writes.empty()) { flags |= POLLOUT; }
            iops.push_back({req.first, flags, {}});
        }

        /// Do the poll dance
        if (::poll(iops.data(), iops.size(), -1) == -1) {
            throw felspar::stdexcept::system_error{
                    errno, std::generic_category(), "poll"};
        }

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
    coro.promise().consume_value();
}


void felspar::poll::poll_warden::cancel(completion *p) { delete p; }


int felspar::poll::poll_warden::create_socket(
        int domain, int type, int protocol) {
    int fd = warden::create_socket(domain, type, protocol);
    felspar::posix::set_non_blocking(fd);
    return fd;
}
