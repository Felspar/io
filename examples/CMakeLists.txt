if(${FELSPAR_ENABLE_IO_URING})
    add_executable(http-benchmark http-benchmark.cpp)
    target_link_libraries(http-benchmark felspar-io)
endif()
