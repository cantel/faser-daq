cmake_minimum_required(VERSION 3.4.3)

project(daqling_top)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wno-unused-function -Wno-unused-variable -O3 -g -pedantic -fPIC")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -ldl")

add_subdirectory(daqling)
include_directories(${daqling_include_dirs})
link_directories(${daqling_link_dirs})

add_subdirectory(src/Modules)
