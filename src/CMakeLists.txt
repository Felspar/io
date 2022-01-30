add_library(felspar-io
        convenience.cpp
        poll.iops.cpp
        poll.warden.cpp
        posix.cpp
        uring.iops.cpp
        uring.warden.cpp
        warden.cpp
    )
target_include_directories(felspar-io PUBLIC ../include)
target_link_libraries(felspar-io PUBLIC felspar-coro uring)
install(TARGETS felspar-io LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(DIRECTORY ../include/felspar DESTINATION include)
