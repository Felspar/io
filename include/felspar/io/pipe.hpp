#pragma once


#include <felspar/io/posix.hpp>


namespace felspar::io {


    /// ## A pipe has a read and a write end
    /**
     * On true POSIX systems this will create a real pipe, but on Windows it
     * will emulate a pipe using a pair of sockets.
     */
    struct pipe {
        posix::fd read, write;
    };


}
