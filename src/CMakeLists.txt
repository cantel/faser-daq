cmake_minimum_required(VERSION 3.4.3)

#library
project(Modules VERSION 0.0.1
        DESCRIPTION "Data acquisition framework"
        LANGUAGES CXX)

include_directories("../include")
include_directories("../daqling/include")

file(GLOB_RECURSE TEST_SOURCES "Test.cpp")
add_library(test SHARED ${TEST_SOURCES})
target_link_libraries(test PUBLIC core utilities zmq)

