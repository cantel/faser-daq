cmake_minimum_required(VERSION 3.4.3)

#library
project(Modules VERSION 0.0.1
        DESCRIPTION "Data acquisition framework"
        LANGUAGES CXX)

include_directories("../../include")
include_directories("../../daqling/include")

#file(GLOB_RECURSE NEWMODULE_SOURCES "NewModule.cpp_template")
#add_library(newmodule SHARED ${NEWMODULE_SOURCES})
#target_link_libraries(newmodule PUBLIC core utilities zmq)

