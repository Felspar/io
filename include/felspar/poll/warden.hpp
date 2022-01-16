#pragma once

#include <felspar/poll/completion.hpp>
#include <felspar/test/source.hpp>

#include <span>

#include <netinet/in.h>


namespace felspar::poll {


    /// Executor that allows `poll` based asynchronous IO
    class warden {
        template<typename R>
        friend struct felspar::poll::iop;

      protected:
        virtual void run_until(felspar::coro::unique_handle<
                               felspar::coro::task_promise<void>>) = 0;
        virtual void cancel(completion *) = 0;

      public:
        virtual ~warden() = default;

        template<typename... PArgs, typename... MArgs>
        void run(coro::task<void> (*f)(warden &, PArgs...), MArgs &&...margs) {
            auto task = f(*this, std::forward<MArgs>(margs)...);
            run_until(task.release());
        }

        /**
         * ### Reading and writing
         */
        /// Read or write bytes from the provided buffer returning the number of
        /// bytes read/written.
        virtual iop<std::size_t> read_some(
                int fd,
                std::span<std::byte>,
                felspar::source_location =
                        felspar::source_location::current()) = 0;
        virtual iop<std::size_t> write_some(
                int fd,
                std::span<std::byte const>,
                felspar::source_location =
                        felspar::source_location::current()) = 0;

        /**
         * ### Socket APIs
         */
        virtual int create_socket(
                int domain,
                int type,
                int protocol,
                felspar::source_location = felspar::source_location::current());

        virtual iop<int>
                accept(int fd,
                       felspar::source_location =
                               felspar::source_location::current()) = 0;
        virtual iop<void>
                connect(int fd,
                        sockaddr const *,
                        socklen_t,
                        felspar::source_location =
                                felspar::source_location::current()) = 0;

        /**
         * ### File readiness
         */
        virtual iop<void> read_ready(
                int fd,
                felspar::source_location =
                        felspar::source_location::current()) = 0;
        virtual iop<void> write_ready(
                int fd,
                felspar::source_location =
                        felspar::source_location::current()) = 0;
    };


    inline iop<void>::~iop() { comp->ward()->cancel(comp); }
    template<typename R>
    inline iop<R>::~iop() {
        comp->ward()->cancel(comp);
    }


}
