#pragma once


#include <felspar/io/warden.hpp>

#include <span>

#include <unistd.h>


namespace felspar::io {


    /// Free standing version of `read_some`
    template<typename S, typename B>
    inline auto read_some(
            io::warden &warden,
            S &&sock,
            B &&buffer,
            std::optional<std::chrono::nanoseconds> const timeout = {},
            felspar::source_location const &loc =
                    felspar::source_location::current()) {
        return warden.read_some(sock, buffer, timeout, loc);
    }


    /// A read buffer that can be split up and have more information read into
    /// it over time as data appears on the file descriptor
    template<typename R>
    class read_buffer {
        R storage = {};
        std::span<typename R::value_type> data_read = {storage.data(), {}};
        std::span<std::byte> empty_buffer = {
                reinterpret_cast<std::byte *>(storage.data()), storage.size()};

      public:
        using storage_type = R;
        using span_type = decltype(data_read);
        using value_type = typename R::value_type;

        read_buffer() {}

        template<typename S>
        warden::task<void> read_some(
                warden &ward,
                S &&sock,
                std::optional<std::chrono::nanoseconds> timeout = {},
                felspar::source_location const &loc =
                        felspar::source_location::current()) {
            std::size_t const bytes_read =
                    co_await ward.read_some(sock, remaining(), timeout, loc);
            if (bytes_read == 0) {
                throw stdexcept::runtime_error{
                        "Got a zero read, we're done", loc};
            } else {
                data_read = {data_read.data(), data_read.size() + bytes_read};
                empty_buffer = empty_buffer.subspan(bytes_read);
            }
        }

        span_type consume(std::size_t const bytes) {
            auto const start = data_read.first(bytes);
            data_read = data_read.subspan(bytes);
            return start;
        }

        /// TODO Almost certainly remove these
        auto not_consumed() const { return data_read; }
        std::span<std::byte> remaining() {
            return {reinterpret_cast<std::byte *>(empty_buffer.data()),
                    empty_buffer.size()};
        }
        auto data() { return storage.data(); }
        auto find(value_type v) {
            return std::find(data_read.begin(), data_read.end(), v);
        }
        auto begin() { return data_read.begin(); }
        auto end() { return data_read.end(); }
    };


    /// Issue a read request for a specific amount of data
    template<typename S>
    inline warden::task<std::size_t> read_exactly(
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
    inline warden::task<std::size_t> read_exactly(
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
    inline warden::task<std::size_t> read_exactly(
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


    /// Read a line (up to the next LF) and strip any final CR
    template<typename S, typename R>
    inline warden::task<typename R::span_type> read_until_lf_strip_cr(
            warden &ward,
            S &&sock,
            R &read_buffer,
            std::optional<std::chrono::nanoseconds> timeout = {},
            felspar::source_location const &loc =
                    felspar::source_location::current()) {
        auto cr = read_buffer.find('\n');
        while (cr == read_buffer.end()) {
            co_await read_buffer.read_some(ward, sock, timeout, loc);
            cr = read_buffer.find('\n');
        }
        std::size_t const line = std::distance(read_buffer.begin(), cr);
        auto read = read_buffer.consume(line + 1).first(line);
        if (read.size() and read.back() == '\r') {
            co_return read.first(read.size() - 1);
        } else {
            co_return read;
        }
    }


}
