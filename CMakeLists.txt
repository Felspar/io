cmake_minimum_required(VERSION 3.10)
project(felspar-io)


if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(FELSPAR_ENABLE_IO_URING YES CACHE BOOL
        "Enable io_uring through liburing")
else()
    set(FELSPAR_ENABLE_IO_URING NO CACHE BOOL
        "Enable io_uring through liburing")
endif()

include(cmake/detect_accept4.cmake)


if(${CMAKE_SOURCE_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
    add_custom_target(felspar-check)
    set_property(TARGET felspar-check PROPERTY EXCLUDE_FROM_ALL TRUE)
    add_custom_target(felspar-stress)
    set_property(TARGET felspar-stress PROPERTY EXCLUDE_FROM_ALL TRUE)
    include(requirements.cmake)
endif()

add_subdirectory(src)

if(TARGET felspar-check)
    add_subdirectory(test)
endif()
