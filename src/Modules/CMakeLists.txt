# Build all the modules
file(GLOB subdirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
foreach(subdir ${subdirs})
  if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})
      if (CMAKE_EMULATION_BUILD)
        if (NOT ${subdir} MATCHES "[Trigger|Tracker|Digitizer]Receiver$")
          add_subdirectory(${subdir})
        else()
          message(STATUS "In EMULATION BUILD: Skipping ${subdir}")
        endif()
      else()
        message(STATUS "adding ${subdir}")
        add_subdirectory(${subdir})
      endif()
  endif()
endforeach()

