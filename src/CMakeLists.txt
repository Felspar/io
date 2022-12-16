add_library(felspar-io
        convenience.cpp
        poll.iops.cpp
        poll.warden.cpp
        posix.cpp
        warden.cpp
    )
target_include_directories(felspar-io PUBLIC ../include)
target_link_libraries(felspar-io PUBLIC felspar-coro)
if(${FELSPAR_ENABLE_IO_URING})
    target_compile_definitions(felspar-io PUBLIC FELSPAR_ENABLE_IO_URING=1)
    target_sources(felspar-io PRIVATE
            uring.iops.cpp
            uring.warden.cpp
        )
    target_link_libraries(felspar-io PUBLIC uring)
endif()
if(${FELSPAR_HAS_ACCEPT4})
    target_compile_definitions(felspar-io PRIVATE FELSPAR_HAS_ACCEPT4=1)
endif()
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    target_link_libraries(felspar-io PUBLIC wsock32 ws2_32)
endif()

install(TARGETS felspar-io LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(DIRECTORY ../include/felspar DESTINATION include)
