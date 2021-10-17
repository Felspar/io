#pragma once


#include <felspar/coro/task.hpp>

#include <span>


namespace felspar::poll {


    class executor;


    /// Write to a file descriptor
    felspar::coro::task<::ssize_t>
            write(executor &, int fd, void const *buf, std::size_t count);

    inline felspar::coro::task<::ssize_t>
            write(executor &e, int fd, std::span<std::uint8_t> s) {
        return write(e, fd, s.data(), s.size());
    }
    inline felspar::coro::task<::ssize_t>
            write(executor &e, int fd, std::span<std::byte> s) {
        return write(e, fd, s.data(), s.size());
    }


}
