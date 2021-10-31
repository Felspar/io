add_library(felspar-poll-headers-tests STATIC EXCLUDE_FROM_ALL
        accept.cpp
        connect.cpp
        poll.cpp
        read.cpp
        warden.cpp
        write.cpp
    )
target_link_libraries(felspar-poll-headers-tests felspar-poll)
add_dependencies(check felspar-poll-headers-tests)
