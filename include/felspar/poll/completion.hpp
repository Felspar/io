#pragma once


#include <felspar/coro/task.hpp>

#include <memory>


namespace felspar::poll {


    template<typename R>
    struct iop;
    class warden;


    template<>
    struct iop<void> {
        struct completion {
            virtual ~completion() = default;
            virtual void await_suspend(
                    felspar::coro::coroutine_handle<> h) noexcept = 0;
        };

        iop(std::unique_ptr<completion> c) : comp{std::move(c)} {}

        bool await_ready() const noexcept { return false; }
        void await_suspend(felspar::coro::coroutine_handle<> h) noexcept {
            comp->await_suspend(h);
        }
        auto await_resume() noexcept {}

      private:
        std::unique_ptr<completion> comp;
    };


}
