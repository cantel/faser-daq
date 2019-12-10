# Define module
daqling_module(module_name)

# Add source file to library
daqling_target_sources(${module_name}
    TrackerReceiverModule.cpp
)

target_include_directories (${module_name} PUBLIC
                            "${daqling_top_SOURCE_DIR}/gpiodrivers/")

#add_library(TRBAccess ${daqling_top_SOURCE_DIR}/gpiodrivers/TRBAccess/src/TRBAccess.cxx)
#add_library(GPIOBase ${daqling_top_SOURCE_DIR}/gpiodrivers/GPIOBase/GPIOBase/GPIOBaseClass.h)

# Maybe whole chain should be added. Add necessary libraries!!!
target_link_libraries (${module_name} PUBLIC
                        TRBAccess)

# Provide install target
daqling_target_install(${module_name})
