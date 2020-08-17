# Define module
daqling_module(module_name)

# Add source file to library
daqling_target_sources(${module_name}
    DigitizerReceiverEmulatorModule.cpp
)

target_include_directories( ${module_name} PUBLIC 
                            "${daqling_top_SOURCE_DIR}/digitizer-readout/digitizer-daq"  
                            "${daqling_top_SOURCE_DIR}/digitizer-readout/digitizer-sis3153-core")
                            
target_link_libraries( ${module_name} PUBLIC  
                        Comm_vx1730
                        Helper 
                        Helper_Event 
                        Helper_sis3153 
                        digitizer-sis3153-core
                        Exceptions )

# Provide install target
daqling_target_install(${module_name})
