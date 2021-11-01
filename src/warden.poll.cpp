#include <felspar/exceptions.hpp>
#include <felspar/poll/warden.poll.hpp>


void felspar::poll::poll_warden::run(
        felspar::coro::unique_handle<felspar::coro::task_promise<void>> coro) {
    coro.resume();
    std::vector<::pollfd> iops;
    std::vector<felspar::coro::coroutine_handle<>> continuations;
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
        for (auto continuation : continuations) { continuation.resume(); }

        /// Garbage collect old coroutines
        live.erase(
                std::remove_if(
                        live.begin(), live.end(),
                        [](auto const &h) {
                            if (h.done()) {
                                h.promise().consume_value();
                                return true;
                            } else {
                                return false;
                            }
                        }),
                live.end());
    }
    coro.promise().consume_value();
}


felspar::poll::iop felspar::poll::poll_warden::read(int fd) {
    return {[this, fd](felspar::coro::coroutine_handle<> h) {
        requests[fd].reads.push_back(h);
    }};
}
felspar::poll::iop felspar::poll::poll_warden::write(int fd) {
    return {[this, fd](felspar::coro::coroutine_handle<> h) {
        requests[fd].writes.push_back(h);
    }};
}
