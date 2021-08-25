cmake_minimum_required(VERSION 3.4.3)

project(TrackerCalibration)

macro(use_cxx11)
if (CMAKE_VERSION VERSION_LESS "3.1")
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -pthread")
endif ()
else ()
set (CMAKE_CXX_STANDARD 11)
endif ()
endmacro(use_cxx11)

macro(use_cxx17)
if (CMAKE_VERSION VERSION_LESS "3.4")
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17 -pthread")
endif ()
else ()
set (CMAKE_CXX_STANDARD 17)
endif ()
endmacro(use_cxx17)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# compiler options and flags
use_cxx11()
#use_cxx17()
#set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
#add_compile_options("-v")
#set(CMAKE_VERBOSE_MAKEFILE ON)

# additional flags for Debug build. The content of CMAKE_CXX_FLAGS_DEBUG 
# is added to the content of CMAKE_CXX_FLAGS
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined")

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# show info
message(STATUS "")
message(STATUS "################################################")
message(STATUS "CMAKE_BUILD_TYPE          = ${CMAKE_BUILD_TYPE}")
message(STATUS "################################################")
message(STATUS "CMAKE_CONFIGURATION_TYPES = ${CMAKE_CONFIGURATION_TYPES}")
message(STATUS "CMAKE_CXX_FLAGS           = ${CMAKE_CXX_FLAGS}")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "CMAKE_CXX_FLAGS_DEBUG     = ${CMAKE_CXX_FLAGS_DEBUG}")
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "CMAKE_CXX_FLAGS_RELEASE   = ${CMAKE_CXX_FLAGS_RELEASE}")
endif()
message(STATUS "################################################")
message(STATUS "")

set(DAQENV FALSE)
if ("${CMAKE_PROJECT_NAME}" STREQUAL "daqling_top")
  message( STATUS "Running in FASER DAQ environment.")
  set(DAQENV TRUE)
endif()

#"Temporary" solution to make trackercalibration compile within FASER DAQ. 
add_compile_options( -fPIC -w -Wno-error=unused-but-set-variable -Wno-error=unused-variable -Wno-error=old-style-cast -Wno-error=unused-parameter -Wno-error=sign-conversion -Wno-error=conversion-null -Wno-error=sign-compare -Wno-error=pedantic -Wno-error=strict-overflow -Wno-error=parentheses -Wno-error=missing-declarations -Wno-error=overflow -Wno-error=unused-parameter -Wno-error=cast-qual)

#
# dependencies
#
# ROOT
find_package(ROOT 6.10 REQUIRED COMPONENTS Hist Tree)
execute_process(COMMAND root-config "--version" OUTPUT_VARIABLE ROOT_VERSION)
message(STATUS "Using ROOT version ${ROOT_VERSION}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ROOT_CXX_FLAGS}")

# curl
find_package(CURL QUIET REQUIRED)

# json and cpr (via DAQling)
include_directories(/opt/json/single_include)
include_directories(/opt/cpr/include)

# build the trackerreadout software that this depends on
if (NOT DAQENV)
  add_subdirectory(gpiodrivers)
  # build the executable applications for the calibration
  add_subdirectory(apps)
endif()

set(TRACKER_READOUT_LIBRARIES GPIOBase TRBAccess usbInterface TRBEventDecoder
ConfigurationHandling Logging Exceptions)

# build the calibration libraries
add_subdirectory(src)





