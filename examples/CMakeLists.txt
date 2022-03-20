if(${FELSPAR_ENABLE_IO_URING})
    add_executable(echo-benchmark echo-benchmark.cpp)
    target_link_libraries(echo-benchmark felspar-io)
endif()
