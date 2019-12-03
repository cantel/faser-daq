cmake_minimum_required(VERSION 3.4.3)

project(daqling_top)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Require a C++17 compliant compiler
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include Threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
FIND_PACKAGE(Threads REQUIRED)

# Enable Boost
set(ENABLE_BOOST ON)
message(STATUS "Building with Boost")

# Build daqling
add_subdirectory(daqling)

# Build modules
include_directories(src)
include_directories(SYSTEM ${daqling_include_dirs})
link_directories(${daqling_link_dirs})

if(NOT BUILD_New)
  set(BUILD_New OFF)
endif()

# Disable some CXX FLAGS for convenience
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error -Wno-zero-as-null-pointer-constant -Wno-undef -Wno-sign-conversion -Wno-old-style-cast -Wno-strict-overflow")

add_subdirectory(src/Modules)
