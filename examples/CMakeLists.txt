if(${FELSPAR_ENABLE_IO_URING})
    add_executable(echo-benchmark echo-benchmark.cpp)
    target_link_libraries(echo-benchmark felspar-io)

    add_executable(http-simple http-simple.cpp)
    target_link_libraries(http-simple felspar-io)
endif()
