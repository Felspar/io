#pragma once


#include <felspar/coro/stream.hpp>
#include <felspar/io/posix.hpp>
#include <felspar/test/source.hpp>


namespace felspar::io {


    class warden;


    /// Produce file descriptors for incoming connections to the provided socket
    felspar::coro::stream<socket_descriptor> accept(
            warden &,
            socket_descriptor,
            felspar::source_location = felspar::source_location::current());
    inline felspar::coro::stream<socket_descriptor>
            accept(warden &w,
                   posix::fd const &sock,
                   felspar::source_location const &loc =
                           felspar::source_location::current()) {
        return accept(w, sock.native_handle(), loc);
    }


}
