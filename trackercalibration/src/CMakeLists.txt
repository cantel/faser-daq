if(APPLE)
   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++11-extensions")
   add_definitions(-D_MACOSX_)
   link_directories(/usr/local/lib/)
elseif(UNIX)
   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
   link_directories(/usr/lib64/)
   link_directories(/opt/cpr/build/lib)
endif()

set(TRACKER_CALIB_SRCS
  Logger.cxx
  Utils.cxx
  Timer.cxx
  Mask.cxx	
  Trim.cxx
  Chip.cxx
  Module.cxx
  CalibManager.cxx
  RunManager.cxx
  ITest.cxx
  L1DelayScan.cxx
  MaskScan.cxx
  ThresholdScan.cxx
  TriggerBurst.cxx	
  StrobeDelay.cxx
  NPointGain.cxx
  TrimScan.cxx
  NoiseOccupancy.cxx
)

add_library(TrackerCalibration ${TRACKER_CALIB_SRCS})
target_include_directories(TrackerCalibration PUBLIC
${CMAKE_CURRENT_SOURCE_DIR}/.. 
${CMAKE_SOURCE_DIR}/gpiodrivers/GPIOBase
${CMAKE_SOURCE_DIR}/gpiodrivers/TRBAccess
${CMAKE_SOURCE_DIR}/faser-common/Logging/include
${CMAKE_SOURCE_DIR}/faser-common/Exceptions
${CMAKE_SOURCE_DIR}/faser-common/EventFormats
${ROOT_INCLUDE_DIRS})
target_link_libraries(TrackerCalibration PUBLIC usb-1.0 cpr curl ${ROOT_LIBRARIES})




