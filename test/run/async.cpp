#include <felspar/io.hpp>
#include <felspar/coro/eager.hpp>
#include <felspar/test.hpp>

#include <array>
#include <vector>


namespace {


    auto const suite = felspar::testsuite("async_resume");


    /**
     * Awaitable that records the handle of the coroutine awaiting it and then
     * stays suspended until something resumes the captured handle.
     */
    struct capture_handle {
        std::vector<std::coroutine_handle<>> *handles;
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> h) { handles->push_back(h); }
        void await_resume() const noexcept {}
    };

    felspar::io::warden::task<void>
            waiter(felspar::io::warden &,
                   std::vector<std::coroutine_handle<>> *const handles,
                   std::size_t *const resumed) {
        co_await capture_handle{handles};
        ++*resumed;
    }


    /**
     * The single handle overload should resume the coroutine, but only once the
     * event loop is pumped.
     */
    template<typename Warden>
    void single(auto check) {
        Warden backend;
        felspar::io::warden &ward = backend;
        std::vector<std::coroutine_handle<>> handles;
        std::size_t resumed = {};

        felspar::io::warden::eager<> w;
        w.post(waiter, std::ref(ward), &handles, &resumed);
        /// The coroutine has run to its suspension point and handed us its handle
        check(handles.size()) == 1uz;
        check(resumed) == 0uz;

        ward.async_resume(handles.front());
        /// Queuing must not resume the coroutine straight away
        check(resumed) == 0uz;

        ward.run_batch();
        /// Draining the loop resumes it exactly once
        check(resumed) == 1uz;

        /// A null handle is silently dropped rather than queued
        ward.async_resume(std::coroutine_handle<>{});
        ward.run_batch();
        check(resumed) == 1uz;
    }


    /**
     * Queue many handles one at a time. Each `async_resume` wakes the loop, so
     * for io_uring this exercises the folding of the repeated wake-up NOPs into
     * a single in-flight CQE. Nothing should resume until the loop is pumped.
     */
    template<typename Warden>
    void multi_single(auto check) {
        Warden backend;
        felspar::io::warden &ward = backend;
        std::size_t constexpr count = 256;
        std::vector<std::coroutine_handle<>> handles;
        std::size_t resumed = {};

        std::vector<felspar::io::warden::eager<>> ws(count);
        for (auto &w : ws) {
            w.post(waiter, std::ref(ward), &handles, &resumed);
        }
        check(handles.size()) == count;

        for (auto h : handles) { ward.async_resume(h); }
        /// Many wakes have happened, but nothing runs until the loop is pumped
        check(resumed) == 0uz;

        ward.run_batch();
        check(resumed) == count;
    }


    /// The span overload should resume every coroutine in the batch.
    template<typename Warden>
    void batch(auto check) {
        Warden backend;
        felspar::io::warden &ward = backend;
        std::vector<std::coroutine_handle<>> handles;
        std::size_t resumed = {};

        std::array<felspar::io::warden::eager<>, 3> ws;
        for (auto &w : ws) {
            w.post(waiter, std::ref(ward), &handles, &resumed);
        }
        check(handles.size()) == 3uz;
        check(resumed) == 0uz;

        ward.async_resume(handles);
        check(resumed) == 0uz;

        ward.run_batch();
        check(resumed) == 3uz;
    }


    /**
     * An `allocator` wrapper forwards `async_resume` to its backing warden, so
     * a coroutine queued through the wrapper is resumed when the backing loop
     * is pumped -- not left stranded in a queue of its own.
     */
    template<typename Warden>
    void via_allocator(auto check) {
        Warden backend;
        felspar::io::allocator wrapper{
                backend, *felspar::pmr::new_delete_resource()};
        felspar::io::warden &ward = wrapper;
        std::vector<std::coroutine_handle<>> handles;
        std::size_t resumed = {};

        felspar::io::warden::eager<> w;
        w.post(waiter, std::ref(ward), &handles, &resumed);
        check(handles.size()) == 1uz;
        check(resumed) == 0uz;

        ward.async_resume(handles.front());
        check(resumed) == 0uz;

        /**
         * Pump the *backing* warden: the wrapper must have forwarded the resume
         * into its queue rather than keeping its own.
         */
        backend.run_batch();
        check(resumed) == 1uz;
    }


    auto const sp = suite.test(
            "single/poll",
            [](auto check) { single<felspar::io::poll_warden>(check); },
            [](auto check) { multi_single<felspar::io::poll_warden>(check); });
    auto const bp = suite.test("batch/poll", [](auto check) {
        batch<felspar::io::poll_warden>(check);
    });
    auto const ap = suite.test("allocator/poll", [](auto check) {
        via_allocator<felspar::io::poll_warden>(check);
    });
#ifdef FELSPAR_ENABLE_IO_URING
    auto const su = suite.test(
            "single/io_uring",
            [](auto check) { single<felspar::io::uring_warden>(check); },
            [](auto check) { multi_single<felspar::io::uring_warden>(check); });
    auto const bu = suite.test("batch/io_uring", [](auto check) {
        batch<felspar::io::uring_warden>(check);
    });
    auto const au = suite.test("allocator/io_uring", [](auto check) {
        via_allocator<felspar::io::uring_warden>(check);
    });
#endif


}
