cmake_minimum_required(VERSION 3.4.3)

#library
project(Modules VERSION 0.0.1
                  DESCRIPTION "Data acquisition framework"
                  LANGUAGES CXX)

include_directories("../../include")

file(GLOB_RECURSE UTILITIES_SOURCES "BoardReader.cpp")
add_library(boardreader SHARED ${UTILITIES_SOURCES})
target_link_libraries(boardreader PUBLIC utilities zmq)

file(GLOB_RECURSE BOARDREADER_SOURCES "EventBuilder.cpp")
add_library(eventbuilder SHARED ${BOARDREADER_SOURCES})
target_link_libraries(eventbuilder PUBLIC core utilities zmq)

file(GLOB_RECURSE BOARDREADER_SOURCES "FileDataLogger.cpp")
add_library(filedatalogger SHARED ${BOARDREADER_SOURCES})
target_link_libraries(filedatalogger PUBLIC core utilities zmq)

file(GLOB_RECURSE BOARDREADER_SOURCES "CassandraDataLogger.cpp")
add_library(cassandradatalogger SHARED ${BOARDREADER_SOURCES})
target_link_libraries(cassandradatalogger PUBLIC core utilities zmq cassandra)

