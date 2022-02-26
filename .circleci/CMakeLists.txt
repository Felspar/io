project(felspar-io-circleci)
cmake_minimum_required(VERSION 3.10)

## Opt in to tests
add_custom_target(felspar-check)
set_property(TARGET felspar-check PROPERTY EXCLUDE_FROM_ALL TRUE)
add_custom_target(felspar-stress)
set_property(TARGET felspar-stress PROPERTY EXCLUDE_FROM_ALL TRUE)
include(../requirements.cmake)

add_subdirectory(.. io)
