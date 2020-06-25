# Define module
daqling_module(module_name)

#target_link_libraries(${module_name} DaqlingModuleMonitor)
#target_sources(${module_name} PRIVATE ../Monitor/MonitorModule.cpp)

target_link_libraries(${module_name} EventFormats)

# Add source file to library
daqling_target_sources(${module_name}
    TrackerMonitorModule.cpp
    ../MonitorBase/MonitorBaseModule.cpp
    ../../Utils/HistogramManager.cpp
)

# Provide install target
daqling_target_install(${module_name})
