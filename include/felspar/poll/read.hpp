#pragma once


#include <felspar/coro/task.hpp>

#include <span>

#include <unistd.h>


namespace felspar::poll {


    class warden;


    /// Issue a read request
    felspar::coro::task<::ssize_t>
            read(warden &, int fd, void *buf, std::size_t count);

    inline felspar::coro::task<ssize_t>
            read(warden &w, int fd, std::span<std::uint8_t> s) {
        return read(w, fd, s.data(), s.size());
    }
    inline felspar::coro::task<ssize_t>
            read(warden &w, int fd, std::span<std::byte> s) {
        return read(w, fd, s.data(), s.size());
    }


}
