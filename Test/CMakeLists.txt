cmake_minimum_required(VERSION 3.4.3)

set(PACKAGE test)
set(PACKAGE_VERSION 0.0.1)

set(VERSION "0.0.0")

## BINARY DISTRIBUTION USING CPACK ##

set(CPACK_RPM_PACKAGE_AUTOREQPROV " no")
set(CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_GENERATOR "RPM" "TGZ")
set(CPACK_PACKAGE_NAME "test")
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_CONTACT "DAQ Developers")
set(CPACK_PACKAGE_VENDOR "CERN")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}.${CMAKE_SYSTEM_PROCESSOR}")
include(CPack)

include(AUTOLOAD)

add_external(spdlog ${spdlog_version})

# CONFIG example:
#option(ENABLE_FIVERBS "enable the fi/verbs backend" ON)
#set(VERSION_STRING ${VERSION})
#configure_file(src/config.h.in config.h NEWLINE_STYLE UNIX)

## COMPILER SETUP ##

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++14 -Wall -g")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
    message(FATAL_ERROR "GCC version must be at least 4.8!")
  endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -qopt-prefetch -unroll-aggressive -march=native -mtune=native")
endif()

## SOURCES AND TARGETS ##

include_directories(${CMAKE_BINARY_DIR})
include_directories(../test)

### DEPENDENCIES

find_package(SPDLOG ${spdlog_version} EXACT REQUIRED)
include_directories(${SPDLOG_INCLUDE_DIRS})
link_directories(${SPDLOG_DIR}/../../)
message(INFO ${SPDLOG_DIR})

#set(TEST_SOURCES src/test_logging.cpp)
#add_library(test SHARED ${NETIO_SOURCES})

#if(ENABLE_FIVERBS)
#  list(APPEND NETIO_SOURCES src/fi_verbs.cpp)
#endif()

add_executable(test_logging src/test_logging.cpp)
target_link_libraries(test_logging utilities)



