add_library(felspar-poll
        iops.poll.cpp
        warden.poll.cpp
    )
target_include_directories(felspar-poll PUBLIC ../include)
target_link_libraries(felspar-poll PUBLIC felspar-coro)
install(TARGETS felspar-poll LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(DIRECTORY ../include/felspar DESTINATION include)
