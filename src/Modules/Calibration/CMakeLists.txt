# Define module
daqling_module(module_name)

# Add source file to library
daqling_target_sources(${module_name}
    CalibrationModule.cpp
)

# In case you need other external libraries (uncomment and modify according for your system)
#target_include_directories (${module_name} PUBLIC <path>/ExternalLibName/include/)
#target_link_libraries(${module_name} PUBLIC ExternalLibName)

target_include_directories (${module_name} PUBLIC
                            ${LIBUSB_INCLUDE_DIR}
                            "${daqling_top_SOURCE_DIR}/gpiodrivers/GPIOBase"
                            "${daqling_top_SOURCE_DIR}/gpiodrivers/TRBAccess"
                            "${daqling_top_SOURCE_DIR}/trackercalibration"
 )

target_link_libraries (${module_name} PUBLIC ${LIBUSB_LIBRARY} TrackerCalibration TRBAccess TRBEventDecoder ConfigurationHandling)

# Provide install target
daqling_target_install(${module_name})