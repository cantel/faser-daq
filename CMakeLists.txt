cmake_minimum_required(VERSION 3.4.3)

set(PACKAGE rd51core)
set(PACKAGE_VERSION 0.0.1)

set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
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
add_external(spdlog ${spdlog_version})
#add_external(docopt ${docopt_version})
#SET(JSON_BuildTests OFF CACHE BOOL "Build the unit tests when BUILD_TESTING is enabled.")
OPTION(JSON_BuildTests "Build the unit tests when BUILD_TESTING is enabled.")
add_external(json ${json_version})
#add_external(cereal ${cereal_version})
#add_external(evpp   ${evpp_version})

SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#library
PROJECT(Utilities)
INCLUDE_DIRECTORIES("Utilities/")
FILE(GLOB_RECURSE UTILITIES_SOURCES "Utilities/src/*.cpp")
ADD_LIBRARY(utilities SHARED ${UTILITIES_SOURCES})

PROJECT(Core)
INCLUDE_DIRECTORIES("Core/")
FILE(GLOB_RECURSE CORE_SOURCES "Core/src/*.cpp")
ADD_LIBRARY(core SHARED ${CORE_SOURCES})

PROJECT(Modules)
INCLUDE_DIRECTORIES("Modules/")
FILE(GLOB_RECURSE BOARDREADER_SOURCES "Modules/src/BoardReader.cpp")
ADD_LIBRARY(boardreader SHARED ${BOARDREADER_SOURCES})
target_link_libraries(boardreader PUBLIC utilities)

FILE(GLOB_RECURSE EVENTBUILDER_SOURCES "Modules/src/EventBuilder.cpp")
ADD_LIBRARY(eventbuilder SHARED ${EVENTBUILDER_SOURCES})
target_link_libraries(eventbuilder PUBLIC utilities)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
    message(FATAL_ERROR "GCC version must be at least 4.8!")
  endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -qopt-prefetch -unroll-aggressive -march=native -mtune=native")
endif()

# Get rid of auto_ptr warnings from yaml-cpp
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

#set(SOURCES src/card_queue_source.cpp src/configuration.cpp src/pipeline.cpp src/random_queue_source.cpp src/ttc_generator_queue_source.cpp src/statistics.cpp src/toflx.cpp src/block_source.cpp src/trace.cpp src/busythread.cpp src/poor_mans_statistics.cpp)

FILE(GLOB_RECURSE RD51_LIBS lib/*.a)

add_executable(rd51 src/main.cpp)
target_link_libraries(rd51 utilities pthread rt)


install(TARGETS rd51 EXPORT DESTINATION bin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -ldl")
add_executable(test_plugin Test/src/test_plugin.cpp)
target_link_libraries(test_plugin PUBLIC pthread rt utilities boardreader eventbuilder)
install(TARGETS test_plugin EXPORT DESTINATION bin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -ldl")
add_executable(test_logging Test/src/test_logging.cpp)
target_link_libraries(test_logging PUBLIC pthread rt utilities core)
install(TARGETS test_logging EXPORT DESTINATION bin)


