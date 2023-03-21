cmake_minimum_required(VERSION 3.4.3)

project(daqling_top)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Require a C++17 compliant compiler
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(FOLLY_NO_CONFIG) # Required for building with Folly library
add_compile_definitions(DONT_LOG_FUNCTION_NAME) # Removes function name from logging entries
# Include Threads
#set(THREADS_PREFER_PTHREAD_FLAG ON)
#FIND_PACKAGE(Threads REQUIRED)
add_compile_definitions(FOLLY_NO_CONFIG) # Required for building with Folly library

# Compile slimmed FASER DAQ (emulation build - no Hardware necessary)
if (NOT CMAKE_EMULATION_BUILD)
  set(CMAKE_EMULATION_BUILD OFF)
elseif(CMAKE_EMULATION_BUILD)
  message(STATUS "FASER EMULATION BUILD - skipping components related to detector hardware!")
endif()

# Enable Boost
set(ENABLE_BOOST ON)
message(STATUS "Building with Boost")

# Build daqling
add_subdirectory(daqling)

# Build modules
include_directories(src)
include_directories(SYSTEM ${daqling_include_dirs})
link_directories(${daqling_link_dirs})

if(NOT BUILD_New)
  set(BUILD_New OFF)
endif()

# Set use of DAQLING_LOGGING
add_compile_definitions(DAQLING_LOGGING)
# Add submodules
add_subdirectory(faser-common)
if (NOT CMAKE_EMULATION_BUILD)
  add_subdirectory(gpiodrivers)
  add_subdirectory(digitizer-readout)
endif()

# Disable some CXX FLAGS for convenience  (if needed)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error -Wno-zero-as-null-pointer-constant -Wno-undef -Wno-sign-conversion -Wno-old-style-cast -Wno-strict-overflow")


add_subdirectory(src/Modules)
