add_library(felspar-poll-headers-tests STATIC EXCLUDE_FROM_ALL
        accept.cpp
        completion.cpp
        connect.cpp
        poll.cpp
        posix.cpp
        post.cpp
        read.cpp
        warden.cpp
        warden.io_uring.cpp
        warden.poll.cpp
        write.cpp
    )
target_link_libraries(felspar-poll-headers-tests felspar-poll)
add_dependencies(check felspar-poll-headers-tests)
