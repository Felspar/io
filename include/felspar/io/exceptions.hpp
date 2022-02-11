#pragma once


#include <felspar/exceptions/messaging_error.hpp>


namespace felspar::io {


    /// Thrown when an IOP times out
    class timeout : public exceptions::messaging_error<std::runtime_error> {
        using superclass = exceptions::messaging_error<std::runtime_error>;

      public:
        using superclass::messaging_error;
    };


}
