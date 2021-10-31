#pragma once


#include <felspar/coro/task.hpp>

#include <netinet/in.h>


namespace felspar::poll {


    class warden;


    /// Asynchronous connect
    felspar::coro::task<void> connect(
            warden &, int fd, const struct sockaddr *, socklen_t addrlen);


}
