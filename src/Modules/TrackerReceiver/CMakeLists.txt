# Define module
daqling_module(module_name)

# Add source file to library
daqling_target_sources(${module_name}
    TrackerReceiverModule.cpp
)

#find_package(libusb-1.0 REQUIRED)

find_path(LIBUSB_INCLUDE_DIR
  NAMES libusb.h
  PATH_SUFFIXES "include" "libusb" "libusb-1.0")
find_library(LIBUSB_LIBRARY
  NAMES libusb-1.0.so
  PATH_SUFFIXES "lib64")

message (${LIBUSB_LIBRARY})
message (${LIBUSB_INCLUDE_DIR})

#target_include_directories(${module_name} ${LIBUSB_INCLUDE_DIR})
#target_link_libraries(${module_name} ${LIBUSB_LIBRARY})

#include_directories(/usr/include/libusb-1.0/)
#link_directories(/usr/lib64/)


target_include_directories (${module_name} PUBLIC
                            ${LIBUSB_INCLUDE_DIR}
                            "${daqling_top_SOURCE_DIR}/gpiodrivers/GPIOBase"
                            "${daqling_top_SOURCE_DIR}/gpiodrivers/TRBAccess"
                            "${daqling_top_SOURCE_DIR}/gpiodrivers/TRBAccess/src/app")

#if (APPLE)
#    target_include_directories(${module_name} PUBLIC "/usr/local/include/libusb-1.0/") 
#elseif(UNIX)
#    target_include_directories(${module_name} PUBLIC "/usr/include/libusb-1.0/")
#endif()

#add_library(TRBAccess1 ${daqling_top_SOURCE_DIR}/gpiodrivers/TRBAccess/src/TRBAccess.cxx)
#add_library(GPIOBasel ${daqling_top_SOURCE_DIR}/gpiodrivers/GPIOBase/src/GPIOBaseClass.cxx)

# Maybe whole chain should be added. Add necessary libraries!!!
#target_link_libraries (TRBAccess)
target_link_libraries (${module_name} PUBLIC ${LIBUSB_LIBRARY} TRBAccess TRBEventDecoder ConfigurationHandling)

# Provide install target
daqling_target_install(${module_name})
