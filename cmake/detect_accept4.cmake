set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
try_compile(FELSPAR_HAS_ACCEPT4 ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/accept4.cpp)