#pragma once


#include <felspar/exceptions/system_error.hpp>


namespace felspar::io {


    /// Thrown when an IOP times out
    class timeout : public stdexcept::system_error {
        using superclass = stdexcept::system_error;

      public:
        using superclass::system_error;
    };


}
