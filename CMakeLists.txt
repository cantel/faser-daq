cmake_minimum_required(VERSION 3.4.3)

project(daqling_top)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Build daqling
add_subdirectory(daqling)

# Build modules
include_directories(${daqling_include_dirs})
link_directories(${daqling_link_dirs})
add_subdirectory(src/Modules)
