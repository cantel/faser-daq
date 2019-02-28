cmake_minimum_required(VERSION 3.4.3)

#library
project(test VERSION 0.0.1
                  DESCRIPTION "Data acquisition framework"
                  LANGUAGES CXX)

include_directories("../include")

add_executable(test_plugin src/test_plugin.cpp)
target_link_libraries(test_plugin PUBLIC pthread rt utilities boardreader eventbuilder)
install(TARGETS test_plugin EXPORT DESTINATION bin)

add_executable(test_logging src/test_logging.cpp)
target_link_libraries(test_logging PUBLIC pthread rt utilities core)
install(TARGETS test_logging EXPORT DESTINATION bin)
