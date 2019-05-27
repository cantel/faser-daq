cmake_minimum_required(VERSION 3.4.3)

#library
project(Modules VERSION 0.0.1
        DESCRIPTION "Data acquisition framework"
        LANGUAGES CXX)

include_directories("../../include")
include_directories("../../daqling/include")

file(GLOB_RECURSE TRIGGERGENERATOR_SOURCES "TriggerGenerator.cpp")
add_library(triggergenerator SHARED ${TRIGGERGENERATOR_SOURCES})
target_link_libraries(triggergenerator PUBLIC core utilities zmq)

file(GLOB_RECURSE FRONTENDEMULATOR_SOURCES "FrontEndEmulator.cpp")
add_library(frontendemulator SHARED ${FRONTENDEMULATOR_SOURCES})
target_link_libraries(frontendemulator PUBLIC core utilities zmq)

file(GLOB_RECURSE FRONTENDRECEIVER_SOURCES "FrontEndReceiver.cpp")
add_library(frontendreceiver SHARED ${FRONTENDRECEIVER_SOURCES})
target_link_libraries(frontendreceiver PUBLIC core utilities zmq)

file(GLOB_RECURSE MONITOR_SOURCES "Monitor.cpp")
add_library(monitor SHARED ${MONITOR_SOURCES})
target_link_libraries(monitor PUBLIC core utilities zmq)

file(GLOB_RECURSE MONITORBINARY_SOURCES "MonitorBinary.cpp")
add_library(monitorbinary SHARED ${MONITORBINARY_SOURCES})
target_link_libraries(monitorbinary PUBLIC core utilities zmq)

file(GLOB_RECURSE EVENTBUILDERFASER_SOURCES "EventBuilderFaser.cpp")
add_library(eventbuilderfaser SHARED ${EVENTBUILDERFASER_SOURCES})
target_link_libraries(eventbuilderfaser PUBLIC core utilities zmq)

file(GLOB_RECURSE NEWMODULE_SOURCES "NewModule.cpp")
add_library(newmodule SHARED ${NEWMODULE_SOURCES})
target_link_libraries(newmodule PUBLIC core utilities zmq)

