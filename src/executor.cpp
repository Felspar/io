#include <felspar/poll/executor.hpp>

#include <iostream>


void felspar::poll::iop::await_suspend(
        felspar::coro::coroutine_handle<> h) noexcept {
    std::cout << "Schedule IOP" << std::endl;
    if (read) {
        exec.requests[fd].reads.push_back(h);
    } else {
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
        std::cout << "Looping" << std::endl;
        /// IOP loop
        iops.clear();
        for (auto const &req : requests) {
            short flags = {};
            if (not req.second.reads.empty()) { flags |= POLLIN; }
            if (req.second.writes.empty()) { flags |= POLLOUT; }
            iops.push_back({req.first, flags, {}});
        }

        /// Do the poll dance
        ::poll(iops.data(), iops.size(), -1);

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
    }
}


felspar::poll::iop felspar::poll::executor::read(int fd) {
    return {*this, fd, true};
}
felspar::poll::iop felspar::poll::executor::write(int fd) {
    return {*this, fd, false};
}
