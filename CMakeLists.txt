cmake_minimum_required(VERSION 3.15)

project(UTrace LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)

if(WIN32)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

add_subdirectory(src)
add_subdirectory(tests)