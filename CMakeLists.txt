cmake_minimum_required(VERSION 3.10)
project(felspar-io)


if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(FELSPAR_ENABLE_IO_URING YES CACHE BOOL
        "Enable io_uring through liburing")
else()
    set(FELSPAR_ENABLE_IO_URING NO CACHE BOOL
        "Enable io_uring through liburing")
endif()


if(${CMAKE_SOURCE_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
    add_custom_target(check)
    add_custom_target(stress)
    include(requirements.cmake)
endif()

add_subdirectory(src)

if(TARGET check)
    set_property(TARGET check PROPERTY EXCLUDE_FROM_ALL TRUE)
    add_subdirectory(test)
endif()
