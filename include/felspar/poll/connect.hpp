#pragma once


#include <felspar/coro/task.hpp>

#include <netinet/in.h>


namespace felspar::poll {


    class executor;


    /// Asynchronous connect
    felspar::coro::task<void>
            connect(executor &exec,
                    int fd,
                    const struct sockaddr *addr,
                    socklen_t addrlen);


}
