cmake_minimum_required(VERSION 3.4.3)

set(PACKAGE utilities)
set(PACKAGE_VERSION 0.0.1)

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
#add_external(cereal ${cereal_version})
#add_external(evpp   ${evpp_version})

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

#add_executable(rd51 src/main.cpp)
#target_link_libraries(rd51 pthread rt)

#install(TARGETS rd51 EXPORT DESTINATION bin)

