cmake_minimum_required(VERSION 3.4.3)

#library
project(Core VERSION 0.0.1
                  DESCRIPTION "Data acquisition framework"
                  LANGUAGES CXX)

include_directories("../../include")
file(GLOB_RECURSE UTILITIES_SOURCES "*.cpp")
add_library(core SHARED ${UTILITIES_SOURCES})

target_link_libraries(core PUBLIC utilities zmq)


#executable
add_executable(main_core main_core.cpp)
target_link_libraries(main_core PUBLIC pthread rt utilities core)
install(TARGETS main_core EXPORT DESTINATION bin)
