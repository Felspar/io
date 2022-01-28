#pragma once


#include <felspar/coro/stream.hpp>
#include <felspar/poll/posix.hpp>
#include <felspar/test/source.hpp>


namespace felspar::poll {


    class warden;


    /// Produce file descriptors for incoming connections to the provided socket
    felspar::coro::stream<int> accept(
            warden &,
            int,
            felspar::source_location = felspar::source_location::current());
    inline felspar::coro::stream<int> accept(
            warden &w,
            posix::fd const &sock,
            felspar::source_location loc = felspar::source_location::current()) {
        return accept(w, sock.native_handle(), std::move(loc));
    }


}
