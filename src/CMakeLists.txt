add_library(felspar-poll
        convenience.cpp
        iops.io_uring.cpp
        iops.poll.cpp
        warden.cpp
        warden.io_uring.cpp
        warden.poll.cpp
    )
target_include_directories(felspar-poll PUBLIC ../include)
target_link_libraries(felspar-poll PUBLIC felspar-coro uring)
install(TARGETS felspar-poll LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(DIRECTORY ../include/felspar DESTINATION include)
