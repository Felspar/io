#pragma once


#include <felspar/io/warden.hpp>


namespace felspar::io {


    using memory_resource = felspar::pmr::memory_resource;


    /// Attach an allocator to the ward so that it can be used to manage IO
    /// coroutine allocations
    class allocator : public warden {
        warden &backing_warden;
        memory_resource &backing_allocator;

      public:
        allocator(warden &bw, memory_resource &ba)
        : backing_warden{bw}, backing_allocator{ba} {}

      private:
        /// Memory related APIs
        void *do_allocate(
                std::size_t const bytes, std::size_t const alignment) override {
            return backing_allocator.allocate(bytes, alignment);
        }
        void do_deallocate(
                void *const p,
                std::size_t const bytes,
                std::size_t const alignment) override {
            backing_allocator.deallocate(p, bytes, alignment);
        }

        bool do_is_equal(memory_resource const &other) const noexcept override {
            return backing_allocator.is_equal(other);
        }

        /// Warden related APIs
        void run_until(felspar::coro::coroutine_handle<> h) override {
            backing_warden.run_until(h);
        }
        iop<void> do_close(
                socket_descriptor const fd,
                felspar::source_location const &loc) override {
            return backing_warden.do_close(fd, loc);
        }
        iop<void> do_sleep(
                std::chrono::nanoseconds const time,
                felspar::source_location const &loc) override {
            return backing_warden.do_sleep(time, loc);
        }
        iop<std::size_t> do_read_some(
                socket_descriptor const fd,
                std::span<std::byte> const buffer,
                std::optional<std::chrono::nanoseconds> const timeout,
                felspar::source_location const &loc) override {
            return backing_warden.do_read_some(fd, buffer, timeout, loc);
        }
        iop<std::size_t> do_write_some(
                socket_descriptor const fd,
                std::span<std::byte const> const buffer,
                std::optional<std::chrono::nanoseconds> const timeout,
                felspar::source_location const &loc) override {
            return backing_warden.do_write_some(fd, buffer, timeout, loc);
        }
        void do_prepare_socket(
                socket_descriptor const sock,
                felspar::source_location const &loc) override {
            return backing_warden.do_prepare_socket(sock, loc);
        }
        iop<socket_descriptor> do_accept(
                socket_descriptor const fd,
                std::optional<std::chrono::nanoseconds> const timeout,
                felspar::source_location const &loc) override {
            return backing_warden.do_accept(fd, timeout, loc);
        }
        iop<void> do_connect(
                socket_descriptor const fd,
                sockaddr const *const addr,
                socklen_t const len,
                std::optional<std::chrono::nanoseconds> const timeout,
                felspar::source_location const &loc) override {
            return backing_warden.do_connect(fd, addr, len, timeout, loc);
        }
        iop<void> do_read_ready(
                socket_descriptor const fd,
                std::optional<std::chrono::nanoseconds> const timeout,
                felspar::source_location const &loc) override {
            return backing_warden.do_read_ready(fd, timeout, loc);
        }
        iop<void> do_write_ready(
                socket_descriptor const fd,
                std::optional<std::chrono::nanoseconds> const timeout,
                felspar::source_location const &loc) override {
            return backing_warden.do_write_ready(fd, timeout, loc);
        }
    };


}
