#pragma once


#include <fcntl.h>
#include <felspar/exceptions/system_error.hpp>


namespace felspar::posix {


    /// Set a file descriptor to non-blocking mode
    inline void set_non_blocking(
            int fd,
            felspar::source_location loc = felspar::source_location::current()) {
        if (int err = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
            err != 0) {
            throw felspar::stdexcept::system_error{
                    errno, std::generic_category(), "fcntl F_SETFL error",
                    std::move(loc)};
        }
    }


}
