cmake_minimum_required(VERSION 3.10)
project(felspar-poll)

if(${CMAKE_SOURCE_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
    add_custom_target(check)
    include(requirements.cmake)
endif()

add_subdirectory(src)

if(TARGET check)
    set_property(TARGET check PROPERTY EXCLUDE_FROM_ALL TRUE)
    add_subdirectory(test)
endif()
