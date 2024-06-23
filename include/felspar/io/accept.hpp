#pragma once


#include <felspar/io/posix.hpp>
#include <felspar/io/warden.hpp>
#include <felspar/test/source.hpp>


namespace felspar::io {


    /// ## `accept`

    /// ### Produce file descriptors for incoming connections
    warden::stream<socket_descriptor> accept(
            warden &,
            socket_descriptor,
            felspar::source_location = felspar::source_location::current());
    FELSPAR_CORO_WRAPPER inline warden::stream<socket_descriptor>
            accept(warden &w,
                   posix::fd const &sock,
                   felspar::source_location const &loc =
                           felspar::source_location::current()) {
        return accept(w, sock.native_handle(), loc);
    }


}
