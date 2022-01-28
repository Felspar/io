#pragma once


#include <felspar/coro/task.hpp>
#include <felspar/poll/warden.hpp>

#include <span>


namespace felspar::poll {


    class warden;


    /// Write all of the buffer to a file descriptor
    template<typename S>
    inline felspar::coro::task<std::size_t> write_all(
            warden &ward,
            S &&sock,
            std::span<std::byte const> s,
            felspar::source_location loc = felspar::source_location::current()) {
        auto out{s};
        while (out.size()) {
            auto const bytes = co_await ward.write_some(sock, out, loc);
            if (not bytes) { co_return s.size() - out.size(); }
            out = out.subspan(bytes);
        }
        co_return s.size();
    }
    template<typename S>
    inline felspar::coro::task<std::size_t> write_all(
            warden &w,
            S &&fd,
            void const *buf,
            std::size_t count,
            felspar::source_location loc = felspar::source_location::current()) {
        return write_all(
                w, std::forward<S>(fd),
                std::span<std::byte const>{
                        reinterpret_cast<std::byte const *>(buf), count},
                std::move(loc));
    }
    template<typename S>
    inline felspar::coro::task<std::size_t> write_all(
            warden &w,
            S &&fd,
            std::span<std::uint8_t const> s,
            felspar::source_location loc = felspar::source_location::current()) {
        return write_all(
                w, std::forward<S>(fd),
                std::span<std::byte const>{
                        reinterpret_cast<std::byte const *>(s.data()), s.size()},
                std::move(loc));
    }


}
