add_library(felspar-poll
        convenience.cpp
        io_uring.iops.cpp
        io_uring.warden.cpp
        poll.iops.cpp
        poll.warden.cpp
        warden.cpp
    )
target_include_directories(felspar-poll PUBLIC ../include)
target_link_libraries(felspar-poll PUBLIC felspar-coro uring)
install(TARGETS felspar-poll LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(DIRECTORY ../include/felspar DESTINATION include)
