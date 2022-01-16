#pragma once


#include <felspar/coro/task.hpp>
#include <felspar/poll/warden.hpp>

#include <span>

#include <unistd.h>


namespace felspar::poll {


    /// Issue a read request for a specific amount of data
    inline felspar::coro::task<std::size_t>
            read_exactly(warden &ward, int fd, std::span<std::byte> s) {
        std::span<std::byte> in{s};
        while (in.size()) {
            auto const bytes = co_await ward.read_some(fd, in);
            if (not bytes) { co_return s.size() - in.size(); }
            in = in.subspan(bytes);
        }
        co_return s.size();
    }
    inline felspar::coro::task<std::size_t>
            read_exactly(warden &w, int fd, void *buf, std::size_t count) {
        return read_exactly(
                w, fd,
                std::span<std::byte>{reinterpret_cast<std::byte *>(buf), count});
    }
    inline felspar::coro::task<std::size_t>
            read_exactly(warden &w, int fd, std::span<std::uint8_t> s) {
        return read_exactly(
                w, fd,
                std::span<std::byte>{
                        reinterpret_cast<std::byte *>(s.data()), s.size()});
    }


}
