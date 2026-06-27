#pragma once


#include <coroutine>
#include <vector>


namespace felspar::io {


    /// ## Deferred coroutine resumption
    /**
     * Holds coroutine handles that are to be resumed once the owning event loop
     * has finished processing its current events. Deferring the resume this way
     * solves problems where resuming a coroutine might destroy the very object
     * that triggered the resumption.
     *
     * Any concrete warden (i.e. one performing IO) should include a member of
     * this type and use it to implement the `async_resume` part of the API.
     */
    class async_resumption {
        std::vector<std::coroutine_handle<>> queued, resuming;

      public:
        /// ### Queue a coroutine handle
        bool queue(std::coroutine_handle<> const h) {
            if (h) {
                queued.push_back(h);
                return true;
            } else {
                return false;
            }
        }
        /**
         * Returns `true` when a non-null handle was queued so the caller knows
         * it needs to wake its event loop.
         */

        /// ### Resume everything currently queued
        void resume_all() {
            std::swap(resuming, queued);
            for (auto h : resuming) { h.resume(); }
            resuming.clear();
        }
        /**
         * Handles queued whilst this runs are left for the next call so that a
         * coroutine that re-queues itself can't starve the event loop.
         */

        /// ### `true` if there are handles waiting to be resumed
        bool has_requests() const noexcept { return not queued.empty(); }
    };


}
