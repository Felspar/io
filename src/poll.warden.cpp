#include "poll.hpp"

#include <felspar/exceptions/system_error.hpp>
#include <felspar/io/posix.hpp>

#include <array>
#include <cstddef>

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
    wakeup = create_pipe();
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
        async_resume_coroutine_handles();
    }
}


void felspar::io::poll_warden::run_batch() {
    clear_timeouts();
    do_poll(0);
    async_resume_coroutine_handles();
}


void felspar::io::poll_warden::wake_event_loop() {
    std::byte const byte{};
#ifdef FELSPAR_WINSOCK2
    [[maybe_unused]] auto const w =
            ::send(wakeup.write.native_handle(),
                   reinterpret_cast<char const *>(&byte), 1, {});
#else
    [[maybe_unused]] auto const w =
            ::write(wakeup.write.native_handle(), &byte, 1);
#endif
}


void felspar::io::poll_warden::drain_wakeup() {
    std::array<std::byte, 64> buffer;
    auto const fd = wakeup.read.native_handle();
#ifdef FELSPAR_WINSOCK2
    while (::recv(fd, reinterpret_cast<char *>(buffer.data()), buffer.size(), {})
           > 0) {}
#else
    while (::read(fd, buffer.data(), buffer.size()) > 0) {}
#endif
}


void felspar::io::poll_warden::do_poll(int const timeout) {
    auto const wake_fd = wakeup.read.native_handle();
    bookkeeping->iops.clear();
    bookkeeping->iops.push_back({wake_fd, POLLIN, {}});
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
#if defined(FELSPAR_WINSOCK2)
        int response = ::WSAPoll(
                bookkeeping->iops.data(),
                static_cast<ULONG>(bookkeeping->iops.size()), timeout);
        if (response < 0) {
            /**
             * Windows seems to return errors here for no good reason, but even
             * if it is for a good reason, we kind of need to keep going anyway
             * (for example if the network is down). We'll let things time out
             * and have the application's error handling take over instead.
             *
             * We'll just pretend that one of the file descriptors was signalled
             * (the number isn't ever used so it's value doesn't matter).
             */
            return 1;
        } else {
            return response;
        }
#else
        return ::poll(
                bookkeeping->iops.data(), bookkeeping->iops.size(), timeout);
#endif
    }();
    if (pr < 0) {
        throw felspar::stdexcept::system_error{
                get_error(), std::system_category(), "poll"};
    } else if (pr > 0) {
        bookkeeping->continuations.clear();
        for (auto events : bookkeeping->iops) {
            if (events.fd == wake_fd) {
                if (events.revents & POLLIN) { drain_wakeup(); }
                continue;
            }
            if (events.revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) {
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
        socket_descriptor sock, std::source_location const loc) {
    felspar::posix::set_non_blocking(sock, loc);
}
