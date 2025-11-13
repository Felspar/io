#pragma once


#include <felspar/io/posix.hpp>
#include <felspar/io/warden.hpp>


namespace felspar::io {


    /// ## `accept`

    /// ### Produce file descriptors for incoming connections
    warden::stream<socket_descriptor>
            accept(warden &,
                   socket_descriptor,
                   std::source_location = std::source_location::current());
    FELSPAR_CORO_WRAPPER inline warden::stream<socket_descriptor> accept(
            warden &w,
            posix::fd const &sock,
            std::source_location const &loc = std::source_location::current()) {
        return accept(w, sock.native_handle(), loc);
    }


}
