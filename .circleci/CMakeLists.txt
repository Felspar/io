project(felspar-io-circleci)
cmake_minimum_required(VERSION 3.10)

## Opt in to tests
add_custom_target(check)
add_custom_target(stress)
include(../requirements.cmake)

add_subdirectory(.. io)
