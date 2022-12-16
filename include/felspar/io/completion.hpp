#pragma once

#include <felspar/coro/task.hpp>
#include <felspar/io/exceptions.hpp>
#include <felspar/test/source.hpp>

#include <memory>
#include <system_error>


namespace felspar::io {


    template<typename R>
    class ec;
    class warden;


    /// Describe the outcome of an API
    template<typename R>
    struct outcome;
    template<>
    struct [[nodiscard]] outcome<void> {
        std::error_code error = {};
        char const *message = "";

        /// Returns true if no error occurred
        explicit operator bool() const noexcept {
            return not static_cast<bool>(error);
        }
        /// Throw the exception associated with the error
        [[noreturn]] void throw_exception(
                felspar::source_location const &loc =
                        felspar::source_location::current()) {
            if (error == timeout::error) {
                throw timeout{message, loc};
            } else {
                throw felspar::stdexcept::system_error{error, message, loc};
            }
        }
        /// Throw the exception if there was one
        void
                value(felspar::source_location const &loc =
                              felspar::source_location::current()) {
            if (error) { throw_exception(loc); }
        }
    };
    template<typename R>
    struct [[nodiscard]] outcome : private outcome<void> {
        using outcome<void>::error;
        using outcome<void>::message;
        using outcome<void>::throw_exception;
        using outcome<void>::operator bool;

        /// Contains the result (if one was recorded)
        std::optional<R> result = {};

        /// Assign a value to the outcome
        outcome &operator=(R r) {
            result = std::move(r);
            return *this;
        }
        outcome &operator=(outcome<void> r) {
            error = r.error;
            message = r.message;
            return *this;
        }

        /// Consume the value, or throw the exception
        R value(felspar::source_location const &loc =
                        felspar::source_location::current()) && {
            if (error) {
                throw_exception(loc);
            } else if (result.has_value()) {
                return std::move(*result);
            } else {
                throw felspar::stdexcept::logic_error{
                        "Optional in outcome was empty", loc};
            }
        }
    };


    /// A completion is always created in response to an IOP request and is used
    /// to track and manage the IOP as it executes
    template<typename R>
    struct [[nodiscard]] completion {
        using result_type = R;

        felspar::coro::coroutine_handle<> handle = {};
        felspar::source_location loc;
        outcome<R> result = {};

        completion(felspar::source_location const &l) : loc{l} {}
        virtual ~completion() = default;

        virtual warden *ward() = 0;
        virtual felspar::coro::coroutine_handle<>
                await_suspend(felspar::coro::coroutine_handle<>) = 0;

        /// Return true if the completion should be destroyed when the iop is
        virtual bool delete_due_to_iop_destructed() = 0;
    };


    /// The awaitable type associated with all IOPs.
    template<typename R>
    class [[nodiscard]] iop {
        template<typename E>
        friend class ec;

      public:
        using result_type = R;
        using completion_type = completion<result_type>;

        iop(completion_type *c) : comp{c} {}
        ~iop();

        iop(iop const &) = delete;
        iop &operator=(iop const &) = delete;
        iop(iop &&i) : comp{std::exchange(i.comp, {})} {}
        iop &operator=(iop &&i) {
            std::swap(i.comp, comp);
            return *this;
        }

        bool await_ready() const noexcept { return false; }
        felspar::coro::coroutine_handle<>
                await_suspend(felspar::coro::coroutine_handle<> h) {
            return comp->await_suspend(h);
        }
        R await_resume() { return std::move(comp->result).value(comp->loc); }

      private:
        completion_type *comp;
    };


}
