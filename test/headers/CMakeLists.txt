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
        warden.uring.cpp
        write.cpp
    )
target_link_libraries(felspar-io-headers-tests felspar-io)
add_dependencies(check felspar-io-headers-tests)
