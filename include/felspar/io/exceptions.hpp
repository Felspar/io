#pragma once


#include <felspar/exceptions/system_error.hpp>


namespace felspar::io {


    /// Thrown when an IOP times out
    class timeout : public stdexcept::system_error {
        using superclass = stdexcept::system_error;

      public:
        timeout(std::string msg, felspar::source_location loc)
        : superclass{
                ETIME, std::generic_category(), std::move(msg),
                std::move(loc)} {}
    };


}
