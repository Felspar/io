#pragma once


#include <felspar/exceptions/system_error.hpp>


namespace felspar::io {


    /// Thrown when an IOP times out
    class timeout : public stdexcept::system_error {
        using superclass = stdexcept::system_error;

      public:
        static std::error_code const error;

        timeout(std::string msg, felspar::source_location loc)
        : superclass{error, std::move(msg), loc} {}
    };
    inline std::error_code const timeout::error = {
            ETIME, std::system_category()};


}
