# Define module
daqling_module(module_name)

# Add source file to library
daqling_target_sources(${module_name}
    TCalibMonitorModule.cpp
)

# In case you need other external libraries (uncomment and modify according for your system)
target_link_libraries(${module_name}  EventFormats)

# Add source file to library
daqling_target_sources(${module_name}
    TCalibMonitorModule.cpp
    ../MonitorBase/MonitorBaseModule.cpp
    ../../Utils/HistogramManager.cpp
)

# Provide install target
daqling_target_install(${module_name})