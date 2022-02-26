add_library(felspar-io-headers-tests STATIC EXCLUDE_FROM_ALL
        accept.cpp
        completion.cpp
        connect.cpp
        error.cpp
        exceptions.cpp
        io.cpp
        posix.cpp
        read.cpp
        warden.cpp
        warden.poll.cpp
        write.cpp
    )
if(${FELSPAR_ENABLE_IO_URING})
    target_sources(felspar-io-headers-tests PRIVATE
            warden.uring.cpp
        )
endif()
target_link_libraries(felspar-io-headers-tests felspar-io)
add_dependencies(felspar-check felspar-io-headers-tests)
