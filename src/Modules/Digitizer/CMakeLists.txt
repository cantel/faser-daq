# Define module
daqling_module(module_name)

# Add source file to library
daqling_target_sources(${module_name}
    DigitizerModule.cpp
)

target_include_directories( ${module_name} PUBLIC 
                            "${daqling_top_SOURCE_DIR}/digitizerreadout/digitizer-daq"  
                            "${daqling_top_SOURCE_DIR}/digitizerreadout/sis3153-core")

# Provide install target
daqling_target_install(${module_name})
