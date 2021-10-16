#pragma once


#include <felspar/coro/stream.hpp>


namespace felspar::poll {


    class executor;


    /// Produce file descriptors for incoming connections to the provided socket
    felspar::coro::stream<int> accept(executor &, int);


}
