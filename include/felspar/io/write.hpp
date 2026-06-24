#pragma once


#include <felspar/coro/task.hpp>
#include <felspar/io/warden.hpp>

#include <span>


namespace felspar::io {


    class warden;


    /// ## Free standing version of `write_some`
    template<typename S>
    inline auto write_some(
            warden &w,
            S &&sock,
            std::span<std::byte const> const s,
            std::optional<deadline> deadline = {},
            std::source_location const loc = std::source_location::current()) {
        return w.write_some(sock, s, deadline, loc);
    }
    template<typename S>
    inline auto write_some(
            warden &w,
            S &&sock,
            std::span<std::byte const> const s,
            std::chrono::nanoseconds const timeout,
            std::source_location const loc = std::source_location::current()) {
        return w.write_some(sock, s, deadline_from(timeout), loc);
    }


    /// ## Try to write data
    /**
     * This performs a synchronous write of as much data as the socket can take
     * at the moment. Write errors results in thrown exceptions.
     */
    std::size_t write_some(socket_descriptor, void const *, std::size_t);


    /// ## Write all of the buffer to a file descriptor
    template<typename S>
    inline warden::task<std::size_t> write_all(
            warden &ward,
            S &&sock,
            std::span<std::byte const> const s,
            std::optional<deadline> deadline = {},
            std::source_location const loc = std::source_location::current()) {
        auto out{s};
        while (out.size()) {
            auto const bytes =
                    co_await write_some(ward, sock, out, deadline, loc);
            /// TODO This calculation for written bytes looks suspiciously wrong
            if (not bytes) { co_return s.size() - out.size(); }
            out = out.subspan(bytes);
        }
        co_return s.size();
    }
    template<typename S>
    FELSPAR_CORO_WRAPPER inline warden::task<std::size_t> write_all(
            warden &ward,
            S &&sock,
            std::span<std::byte const> const s,
            std::chrono::nanoseconds const timeout,
            std::source_location const loc = std::source_location::current()) {
        return write_all(
                ward, std::forward<S>(sock), s, deadline_from(timeout), loc);
    }
    template<typename S>
    FELSPAR_CORO_WRAPPER inline warden::task<std::size_t> write_all(
            warden &w,
            S &&fd,
            void const *buf,
            std::size_t count,
            std::optional<deadline> deadline = {},
            std::source_location const loc = std::source_location::current()) {
        return write_all(
                w, std::forward<S>(fd),
                std::span<std::byte const>{
                        reinterpret_cast<std::byte const *>(buf), count},
                deadline, loc);
    }
    template<typename S>
    FELSPAR_CORO_WRAPPER inline warden::task<std::size_t> write_all(
            warden &w,
            S &&fd,
            void const *buf,
            std::size_t count,
            std::chrono::nanoseconds const timeout,
            std::source_location const loc = std::source_location::current()) {
        return write_all(
                w, std::forward<S>(fd),
                std::span<std::byte const>{
                        reinterpret_cast<std::byte const *>(buf), count},
                deadline_from(timeout), loc);
    }
    template<typename S>
    FELSPAR_CORO_WRAPPER inline warden::task<std::size_t> write_all(
            warden &w,
            S &&fd,
            std::span<std::uint8_t const> s,
            std::optional<deadline> deadline = {},
            std::source_location const loc = std::source_location::current()) {
        return write_all(
                w, std::forward<S>(fd),
                std::span<std::byte const>{
                        reinterpret_cast<std::byte const *>(s.data()), s.size()},
                deadline, loc);
    }
    template<typename S>
    FELSPAR_CORO_WRAPPER inline warden::task<std::size_t> write_all(
            warden &w,
            S &&fd,
            std::span<std::uint8_t const> s,
            std::chrono::nanoseconds const timeout,
            std::source_location const loc = std::source_location::current()) {
        return write_all(
                w, std::forward<S>(fd),
                std::span<std::byte const>{
                        reinterpret_cast<std::byte const *>(s.data()), s.size()},
                deadline_from(timeout), loc);
    }
    template<typename S>
    FELSPAR_CORO_WRAPPER inline warden::task<std::size_t> write_all(
            warden &w,
            S &&fd,
            std::string_view s,
            std::optional<deadline> deadline = {},
            std::source_location const loc = std::source_location::current()) {
        return write_all(
                w, std::forward<S>(fd),
                std::span<std::byte const>{
                        reinterpret_cast<std::byte const *>(s.data()), s.size()},
                deadline, loc);
    }
    template<typename S>
    FELSPAR_CORO_WRAPPER inline warden::task<std::size_t> write_all(
            warden &w,
            S &&fd,
            std::string_view s,
            std::chrono::nanoseconds const timeout,
            std::source_location const loc = std::source_location::current()) {
        return write_all(
                w, std::forward<S>(fd),
                std::span<std::byte const>{
                        reinterpret_cast<std::byte const *>(s.data()), s.size()},
                deadline_from(timeout), loc);
    }


}
