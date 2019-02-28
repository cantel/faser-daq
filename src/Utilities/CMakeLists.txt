cmake_minimum_required(VERSION 3.4.3)

#library
project(Utilities VERSION 0.0.1
                  DESCRIPTION "Data acquisition framework"
                  LANGUAGES CXX)

include_directories("../../include")
file(GLOB_RECURSE UTILITIES_SOURCES "*.cpp")
add_library(utilities SHARED ${UTILITIES_SOURCES})
