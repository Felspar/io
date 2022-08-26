#pragma once


#include <felspar/io/posix.hpp>
#include <felspar/io/warden.hpp>
#include <felspar/test/source.hpp>


namespace felspar::io {


    /// Produce file descriptors for incoming connections to the provided socket
    warden::stream<int> accept(
            warden &,
            int,
            felspar::source_location = felspar::source_location::current());
    inline warden::stream<int>
            accept(warden &w,
                   posix::fd const &sock,
                   felspar::source_location const &loc =
                           felspar::source_location::current()) {
        return accept(w, sock.native_handle(), loc);
    }


}
