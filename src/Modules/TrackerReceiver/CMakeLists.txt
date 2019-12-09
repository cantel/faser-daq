# Define module
daqling_module(module_name)

# Add source file to library
daqling_target_sources(${module_name}
    TRBModule.cpp
)

target_include_directories (${module_name} PUBLIC
                            "${daqling_top_SOURCE_DIR}/gpiodrivers/TRBAccess/TrackerReadout")

# Maybe whole chain should be added. Add necessary libraries!!!
target_link_libraries (${module_name} PUBLIC
                        TRBAccess)

# Provide install target
daqling_target_install(${module_name})
