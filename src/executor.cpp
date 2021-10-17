#include <felspar/exceptions.hpp>
#include <felspar/poll/executor.hpp>

#include <iostream>


void felspar::poll::iop::await_suspend(
        felspar::coro::coroutine_handle<> h) noexcept {
    if (read) {
    std::cout << "Schedule read IOP" << std::endl;
        exec.requests[fd].reads.push_back(h);
    } else {
    std::cout << "Schedule write IOP" << std::endl;
        exec.requests[fd].writes.push_back(h);
    }
}


void felspar::poll::executor::run(
        felspar::coro::unique_handle<felspar::coro::task_promise<void>> coro) {
    std::cout << "Running" << std::endl;
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


felspar::poll::iop felspar::poll::executor::read(int fd) {
    return {*this, fd, true};
}
felspar::poll::iop felspar::poll::executor::write(int fd) {
    return {*this, fd, false};
}
