# Define module
daqling_module(module_name)

# Add source file to library
daqling_target_sources(${module_name}
    TLBMonitorModule.cpp
    ../Monitor/MonitorModule.cpp
    ../../Utils/HistogramManager.cpp
)

#target_link_libraries(${module_name} ${FASERTOOLS_LIBRARIES})

# Provide install target
daqling_target_install(${module_name})
