# Define module
daqling_module(module_name)

target_link_libraries(${module_name} EventFormats)

message("Current DigitizerMonitor :  ${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${module_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../faser-common)



# Add source file to library
daqling_target_sources(${module_name}
    DigitizerMonitorModule.cpp
    ../Monitor/MonitorModule.cpp
    ../../Utils/HistogramManager.cpp
)

# Provide install target
daqling_target_install(${module_name})






