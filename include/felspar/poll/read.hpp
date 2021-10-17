#pragma once


#include <felspar/coro/task.hpp>

#include <span>

#include <unistd.h>


namespace felspar::poll {


    class executor;


    /// Issue a read request
    felspar::coro::task<::ssize_t>
            read(executor &, int fd, void *buf, std::size_t count);

    inline felspar::coro::task<ssize_t>
            read(executor &e, int fd, std::span<std::uint8_t> s) {
        return read(e, fd, s.data(), s.size());
    }
    inline felspar::coro::task<ssize_t>
            read(executor &e, int fd, std::span<std::byte> s) {
        return read(e, fd, s.data(), s.size());
    }


}
