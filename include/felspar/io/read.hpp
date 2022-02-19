#pragma once


#include <felspar/coro/task.hpp>
#include <felspar/io/warden.hpp>

#include <span>

#include <unistd.h>


namespace felspar::io {


    /// Issue a read request for a specific amount of data
    template<typename S>
    inline felspar::coro::task<std::size_t> read_exactly(
            warden &ward,
            S &&sock,
            std::span<std::byte> b,
            std::optional<std::chrono::nanoseconds> timeout = {},
            felspar::source_location const &loc =
                    felspar::source_location::current()) {
        std::span<std::byte> in{b};
        while (in.size()) {
            auto const bytes = co_await ward.read_some(sock, in, timeout, loc);
            if (not bytes) { co_return b.size() - in.size(); }
            in = in.subspan(bytes);
        }
        co_return b.size();
    }
    template<typename S>
    inline felspar::coro::task<std::size_t> read_exactly(
            warden &w,
            S &&s,
            void *buf,
            std::size_t count,
            std::optional<std::chrono::nanoseconds> timeout = {},
            felspar::source_location const &loc =
                    felspar::source_location::current()) {
        return read_exactly(
                w, std::forward<S>(s),
                std::span<std::byte>{reinterpret_cast<std::byte *>(buf), count},
                std::move(timeout), loc);
    }
    template<typename S>
    inline felspar::coro::task<std::size_t> read_exactly(
            warden &w,
            S &&s,
            std::span<std::uint8_t> b,
            std::optional<std::chrono::nanoseconds> timeout = {},
            felspar::source_location const &loc =
                    felspar::source_location::current()) {
        return read_exactly(
                w, std::forward<S>(s),
                std::span<std::byte>{
                        reinterpret_cast<std::byte *>(b.data()), b.size()},
                std::move(timeout), loc);
    }


}
