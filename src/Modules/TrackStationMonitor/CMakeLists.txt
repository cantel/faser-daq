# Define module
daqling_module(module_name)

find_package(Eigen3 REQUIRED)
target_link_libraries(${module_name} EventFormats Eigen3::Eigen)

# Add source file to library
daqling_target_sources(${module_name}
    TrackStationMonitorModule.cpp
    ../MonitorBase/MonitorBaseModule.cpp
    ../../Utils/HistogramManager.cpp
)

# Provide install target
daqling_target_install(${module_name})
