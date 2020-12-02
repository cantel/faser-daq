cmake_minimum_required(VERSION 3.10)

# set the project name
project(Runnumber)

#include(FetchContent)
#FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/whoshuu/cpr.git GIT_TAG c8d33915dbd88ad6c92b258869b03aba06587ff9) # the commit hash for 1.5.0
#FetchContent_MakeAvailable(cpr)
find_package(CURL QUIET REQUIRED)

#for machines with DAQling installed
include_directories(SYSTEM /opt/json/single_include/nlohmann)
include_directories(SYSTEM /opt/cpr/include)
link_directories(/opt/cpr/build/lib)

# add the executable
add_executable(Runnumber runnumber.cxx)
target_include_directories(Runnumber PRIVATE /home/aagaard/FASER/TDAQ/RunnumberService/c++)
target_link_libraries(Runnumber PRIVATE cpr curl)
