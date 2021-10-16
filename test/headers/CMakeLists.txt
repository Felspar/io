add_library(felspar-poll-headers-tests STATIC EXCLUDE_FROM_ALL
        accept.cpp
        executor.cpp
        poll.cpp
    )
target_link_libraries(felspar-poll-headers-tests felspar-poll)
add_dependencies(check felspar-poll-headers-tests)
