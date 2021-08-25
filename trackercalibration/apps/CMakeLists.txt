# CXX flags
if(APPLE)
   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++11-extensions")
   link_directories(/usr/local/lib/)
elseif(UNIX)
   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
   link_directories(/opt/cpr/build/lib)
endif()

# targets
add_executable(safePhaseDetection safePhaseDetection.cxx)
target_link_libraries(safePhaseDetection TrackerCalibration GPIOBase TRBAccess usbInterface TRBEventDecoder ConfigurationHandling)

add_executable(tcalib tcalib.cxx)
target_link_libraries(tcalib TrackerCalibration GPIOBase TRBAccess usbInterface TRBEventDecoder ConfigurationHandling cpr curl)

add_executable(thscan thscan.cxx)
target_link_libraries(thscan TrackerCalibration GPIOBase TRBAccess usbInterface TRBEventDecoder ConfigurationHandling cpr curl)

add_executable(tburst tburst.cxx)
target_link_libraries(tburst TrackerCalibration GPIOBase TRBAccess usbInterface TRBEventDecoder ConfigurationHandling cpr curl)

add_executable(decodeCfg decodeCfg.cxx)
target_link_libraries(decodeCfg TrackerCalibration GPIOBase TRBAccess usbInterface TRBEventDecoder ConfigurationHandling cpr curl)

add_executable(trbfirm trbfirm.cxx)
target_include_directories(trbfirm PUBLIC 
${CMAKE_CURRENT_SOURCE_DIR}/.. 
${CMAKE_CURRENT_SOURCE_DIR}/../gpiodrivers/GPIOBase
${CMAKE_CURRENT_SOURCE_DIR}/../gpiodrivers/TRBAccess
)
target_link_libraries(trbfirm PUBLIC GPIOBase TRBAccess usbInterface TRBEventDecoder ConfigurationHandling usb-1.0)

install(TARGETS tcalib thscan trbfirm decodeCfg RUNTIME DESTINATION bin)








