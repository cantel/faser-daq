cmake_minimum_required(VERSION 3.4.3)

#library
project(Modules VERSION 0.0.1
        DESCRIPTION "Data acquisition framework"
        LANGUAGES CXX)

include_directories("../../include")

file(GLOB_RECURSE BOARDREADER_SOURCES "BoardReader.cpp")
add_library(boardreader SHARED ${BOARDREADER_SOURCES})
target_link_libraries(boardreader PUBLIC core utilities zmq)

file(GLOB_RECURSE BOARDREADERBINARY_SOURCES "BoardReaderBinary.cpp")
add_library(boardreaderbinary SHARED ${BOARDREADERBINARY_SOURCES})
target_link_libraries(boardreaderbinary PUBLIC core utilities zmq)

file(GLOB_RECURSE EVENTBUILDER_SOURCES "EventBuilder.cpp")
add_library(eventbuilder SHARED ${EVENTBUILDER_SOURCES})
target_link_libraries(eventbuilder PUBLIC core utilities zmq)

file(GLOB_RECURSE EVENTBUILDERBINARY_SOURCES "EventBuilderBinary.cpp")
add_library(eventbuilderbinary SHARED ${EVENTBUILDERBINARY_SOURCES})
target_link_libraries(eventbuilderbinary PUBLIC core utilities zmq)

file(GLOB_RECURSE FILEDATALOGGER_SOURCES "FileDataLogger.cpp")
add_library(filedatalogger SHARED ${FILEDATALOGGER_SOURCES})
target_link_libraries(filedatalogger PUBLIC core utilities zmq)

file(GLOB_RECURSE CASSANDRADATALOGGER_SOURCES "CassandraDataLogger.cpp")
add_library(cassandradatalogger SHARED ${CASSANDRADATALOGGER_SOURCES})
target_link_libraries(cassandradatalogger PUBLIC core utilities zmq cassandra)

