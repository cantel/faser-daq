# Define module
daqling_module(module_name)

target_link_libraries(${module_name} EventFormats)

# Add source file to library
daqling_target_sources(${module_name}
    MonitorModule.cpp
    ../../Utils/HistogramManager.cpp
)

# Provide install target
daqling_target_install(${module_name})
