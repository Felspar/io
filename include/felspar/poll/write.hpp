#pragma once


#include <felspar/coro/task.hpp>
#include <felspar/poll/warden.hpp>

#include <span>


namespace felspar::poll {


    class warden;


    /// Write all of the buffer to a file descriptor
    inline felspar::coro::task<std::size_t> write_all(
            warden &ward,
            int fd,
            std::span<std::byte const> s,
            felspar::source_location = felspar::source_location::current()) {
        auto out{s};
        while (out.size()) {
            auto const bytes = co_await ward.write_some(fd, out);
            if (not bytes) { co_return s.size() - out.size(); }
            out = out.subspan(bytes);
        }
        co_return s.size();
    }
    inline felspar::coro::task<std::size_t> write_all(
            warden &w,
            int fd,
            void const *buf,
            std::size_t count,
            felspar::source_location = felspar::source_location::current()) {
        return write_all(
                w, fd,
                std::span<std::byte const>{
                        reinterpret_cast<std::byte const *>(buf), count});
    }
    inline felspar::coro::task<std::size_t> write_all(
            warden &w,
            int fd,
            std::span<std::uint8_t const> s,
            felspar::source_location = felspar::source_location::current()) {
        return write_all(
                w, fd,
                std::span<std::byte const>{
                        reinterpret_cast<std::byte const *>(s.data()),
                        s.size()});
    }


}
