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
#set(THREADS_PREFER_PTHREAD_FLAG ON)
#FIND_PACKAGE(Threads REQUIRED)
add_compile_definitions(FOLLY_NO_CONFIG) # Required for building with Folly library
#link custom datatypes to daqling
if(EXISTS ${CMAKE_SOURCE_DIR}/src/DataTypes)
  set(CUSTOM_DATATYPES_DIR ${CMAKE_SOURCE_DIR}/src/DataTypes)
  set(CUSTOM_DATATYPES true)
  add_compile_definitions(CUSTOM_DATATYPES)
endif()
# Build daqling
add_subdirectory(daqling)

# Build modules
include_directories(src)
include_directories(SYSTEM ${daqling_include_dirs})
link_directories(${daqling_link_dirs})

if(NOT BUILD_New)
  set(BUILD_New OFF)
endif()

# Disable some CXX FLAGS for convenience (if needed)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error")

add_subdirectory(src/Modules)
