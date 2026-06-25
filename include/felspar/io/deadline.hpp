#pragma once


#include <chrono>


namespace felspar::io {


    /// ## Deadlines
    using deadline = std::chrono::steady_clock::time_point;
    /**
     * A `deadline` is the absolute point in time by which an operation must
     * complete. A missing deadline (`std::nullopt`) means the operation has no
     * deadline and may run forever.
     */


    /// ### Conversion
    inline deadline deadline_from(std::chrono::nanoseconds timeout) {
        return std::chrono::steady_clock::now() + timeout;
    }


}
