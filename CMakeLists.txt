cmake_minimum_required(VERSION 3.4.3)

project(rd51 VERSION 0.0.1
                  DESCRIPTION "Data acquisition framework"
                  LANGUAGES CXX)

set(PACKAGE rd51core)
set(PACKAGE_VERSION 0.0.1)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
    message(FATAL_ERROR "GCC version must be at least 4.8!")
  endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -qopt-prefetch -unroll-aggressive -march=native -mtune=native")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_CXX_FLAGS -DGIT_DESCRIBE="`cd ${CMAKE_CURRENT_SOURCE_DIR}; git describe --tags --always --dirty`")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -O3 -g -pedantic")

include(AUTOLOAD)

include_directories(src)

#find_package(TBB ${tbb_version} EXACT REQUIRED)
#include_directories(${TBB_INCLUDE_DIRS})
#link_directories(${TBB_DIR}/lib)
#
#find_package(Boost ${boost_version} EXACT REQUIRED COMPONENTS system filesystem regex)
#include_directories(${Boost_INCLUDE_DIRS})

# ZeroMQ
#felix_add_external(zeromq ${zeromq_version} ${BINARY_TAG})
# No FindZeroMQ.cmake available yet
# find_package(ZeroMQ ${zeromq_version} EXACT REQUIRED)
#include_directories(${ZEROMQ_DIR}/include)
#link_directories(${ZEROMQ_DIR}/lib)

# others
add_external(spdlog ${spdlog_version}) # A spd log version is installed along with CentOS but is too old

#add_external(docopt ${docopt_version})

#set(JSON_BuildTests OFF CACHE BOOL "Build the unit tests when BUILD_TESTING is enabled.")
option(JSON_BuildTests "Build the unit tests when BUILD_TESTING is enabled.")
add_external(json ${json_version})

#add_external(cereal ${cereal_version})
#add_external(evpp   ${evpp_version})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(src/Utilities)
add_subdirectory(src/Core)
add_subdirectory(src/Modules)

# Get rid of auto_ptr warnings from yaml-cpp
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

FILE(GLOB_RECURSE RD51_LIBS lib/*.a)

include_directories("include")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -ldl")
add_executable(test_plugin test/src/test_plugin.cpp)



target_link_libraries(test_plugin PUBLIC pthread rt utilities boardreader eventbuilder)
install(TARGETS test_plugin EXPORT DESTINATION bin)

add_executable(test_logging test/src/test_logging.cpp)
target_link_libraries(test_logging PUBLIC pthread rt utilities core)
install(TARGETS test_logging EXPORT DESTINATION bin)
