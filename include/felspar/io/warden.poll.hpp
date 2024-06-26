#pragma once


#include <felspar/io/warden.hpp>

#include <map>
#include <vector>


namespace felspar::io {


    /// ## `poll` based warden
    /**
     * This warden is compatible with both most POSIX systems and Windows
     * (through the use of `WSAPoll`).
     *
     * Creation of any `poll_warden` instance will turn on ignoring of the
     * `SIGPIPE` signal. This allows the `write` calls to return errors without
     * also needing to install signal handler.
     */
    class poll_warden : public warden {
        struct retrier;
        template<typename R>
        struct completion;
        struct request {
            std::vector<retrier *> reads, writes;
        };
        std::map<socket_descriptor, request> requests;
        std::multimap<std::chrono::steady_clock::time_point, retrier *> timeouts;

        void run_until(felspar::coro::coroutine_handle<>) override;

        struct close_completion;
        struct sleep_completion;
        struct read_some_completion;
        struct write_some_completion;
        struct accept_completion;
        struct connect_completion;
        struct read_ready_completion;
        struct write_ready_completion;


      public:
        poll_warden();
        ~poll_warden();

        void run_batch() override;


      protected:
        /// ### File descriptors
        iop<void> do_close(
                socket_descriptor fd,
                felspar::source_location const &) override;

        /// ### Time management
        iop<void> do_sleep(
                std::chrono::nanoseconds,
                felspar::source_location const &) override;

        /// ### Read & write
        iop<std::size_t> do_read_some(
                socket_descriptor fd,
                std::span<std::byte>,
                std::optional<std::chrono::nanoseconds>,
                felspar::source_location const &) override;
        iop<std::size_t> do_write_some(
                socket_descriptor fd,
                std::span<std::byte const>,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) override;

        /// ### Sockets
        void do_prepare_socket(
                socket_descriptor sock,
                felspar::source_location const &) override;
        iop<socket_descriptor> do_accept(
                socket_descriptor fd,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) override;
        iop<void> do_connect(
                socket_descriptor fd,
                sockaddr const *,
                socklen_t,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) override;

        /// ### File descriptor readiness
        iop<void> do_read_ready(
                socket_descriptor fd,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) override;
        iop<void> do_write_ready(
                socket_descriptor fd,
                std::optional<std::chrono::nanoseconds> timeout,
                felspar::source_location const &) override;


      private:
        /// Used for managing the poll loop
        struct loop_data;
        std::unique_ptr<loop_data> bookkeeping;

        /**
         * Resume any coros that have now timed out and return the time out
         * number to pass to poll
         */
        int clear_timeouts();
        /// Put together data for poll call and then process resulting `revents`
        void do_poll(int timeout);
    };


}
