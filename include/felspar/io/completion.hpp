#pragma once

#include <felspar/coro/task.hpp>
#include <felspar/io/exceptions.hpp>
#include <felspar/test/source.hpp>

#include <memory>
#include <system_error>


namespace felspar::io {


    template<typename R>
    class ec;
    template<typename R>
    class result;
    class warden;


    namespace detail {
        struct error_store {
            felspar::source_location loc;
            std::error_code code = {};
            char const *message = "";
            [[noreturn]] void throw_exception() {
                if (code == timeout::error) {
                    throw timeout{message, std::move(loc)};
                } else {
                    throw felspar::stdexcept::system_error{
                            code, message, std::move(loc)};
                }
            }
            explicit operator bool() const noexcept {
                return static_cast<bool>(code);
            }
        };
    }

    template<typename R>
    struct completion {
        using result_type = R;

        std::size_t iop_count = 1;
        felspar::coro::coroutine_handle<> handle;
        detail::error_store error;
        R result = {};

        completion(felspar::source_location l) : error{std::move(l)} {}
        virtual ~completion() = default;

        virtual warden *ward() = 0;
        virtual felspar::coro::coroutine_handle<>
                await_suspend(felspar::coro::coroutine_handle<>) = 0;
    };
    template<>
    struct completion<void> {
        using result_type = void;

        std::size_t iop_count = 1;
        felspar::coro::coroutine_handle<> handle;
        detail::error_store error;

        completion(felspar::source_location l) : error{std::move(l)} {}
        virtual ~completion() = default;

        virtual warden *ward() = 0;
        virtual felspar::coro::coroutine_handle<>
                await_suspend(felspar::coro::coroutine_handle<> h) = 0;
    };


    template<typename R>
    class iop {
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
        R await_resume() {
            if (comp->error) {
                comp->error.throw_exception();
            } else {
                return std::move(comp->result);
            }
        }

      private:
        completion_type *comp;
    };
    template<>
    class iop<void> {
        template<typename E>
        friend class ec;

      public:
        using result_type = void;
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
        auto await_resume() {
            if (comp->error) { comp->error.throw_exception(); }
        }

      private:
        completion_type *comp;
    };


}
