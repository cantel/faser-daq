# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.13

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake3

# The command to remove a file.
RM = /usr/bin/cmake3 -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/engamber/workspace/daqling_top

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/engamber/workspace/daqling_top/build

# Include any dependencies generated for this target.
include daqling/src/Modules/CMakeFiles/eventbuilderbinary.dir/depend.make

# Include the progress variables for this target.
include daqling/src/Modules/CMakeFiles/eventbuilderbinary.dir/progress.make

# Include the compile flags for this target's objects.
include daqling/src/Modules/CMakeFiles/eventbuilderbinary.dir/flags.make

daqling/src/Modules/CMakeFiles/eventbuilderbinary.dir/EventBuilderBinary.cpp.o: daqling/src/Modules/CMakeFiles/eventbuilderbinary.dir/flags.make
daqling/src/Modules/CMakeFiles/eventbuilderbinary.dir/EventBuilderBinary.cpp.o: ../daqling/src/Modules/EventBuilderBinary.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/engamber/workspace/daqling_top/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object daqling/src/Modules/CMakeFiles/eventbuilderbinary.dir/EventBuilderBinary.cpp.o"
	cd /home/engamber/workspace/daqling_top/build/daqling/src/Modules && /cvmfs/sft.cern.ch/lcg/releases/gcc/6.2.0-b9934/x86_64-centos7/bin/g++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/eventbuilderbinary.dir/EventBuilderBinary.cpp.o -c /home/engamber/workspace/daqling_top/daqling/src/Modules/EventBuilderBinary.cpp

daqling/src/Modules/CMakeFiles/eventbuilderbinary.dir/EventBuilderBinary.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/eventbuilderbinary.dir/EventBuilderBinary.cpp.i"
	cd /home/engamber/workspace/daqling_top/build/daqling/src/Modules && /cvmfs/sft.cern.ch/lcg/releases/gcc/6.2.0-b9934/x86_64-centos7/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/engamber/workspace/daqling_top/daqling/src/Modules/EventBuilderBinary.cpp > CMakeFiles/eventbuilderbinary.dir/EventBuilderBinary.cpp.i

daqling/src/Modules/CMakeFiles/eventbuilderbinary.dir/EventBuilderBinary.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/eventbuilderbinary.dir/EventBuilderBinary.cpp.s"
	cd /home/engamber/workspace/daqling_top/build/daqling/src/Modules && /cvmfs/sft.cern.ch/lcg/releases/gcc/6.2.0-b9934/x86_64-centos7/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/engamber/workspace/daqling_top/daqling/src/Modules/EventBuilderBinary.cpp -o CMakeFiles/eventbuilderbinary.dir/EventBuilderBinary.cpp.s

# Object files for target eventbuilderbinary
eventbuilderbinary_OBJECTS = \
"CMakeFiles/eventbuilderbinary.dir/EventBuilderBinary.cpp.o"

# External object files for target eventbuilderbinary
eventbuilderbinary_EXTERNAL_OBJECTS =

lib/libeventbuilderbinary.so: daqling/src/Modules/CMakeFiles/eventbuilderbinary.dir/EventBuilderBinary.cpp.o
lib/libeventbuilderbinary.so: daqling/src/Modules/CMakeFiles/eventbuilderbinary.dir/build.make
lib/libeventbuilderbinary.so: lib/libcore.so
lib/libeventbuilderbinary.so: lib/libutilities.so
lib/libeventbuilderbinary.so: daqling/src/Modules/CMakeFiles/eventbuilderbinary.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/engamber/workspace/daqling_top/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX shared library ../../../lib/libeventbuilderbinary.so"
	cd /home/engamber/workspace/daqling_top/build/daqling/src/Modules && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/eventbuilderbinary.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
daqling/src/Modules/CMakeFiles/eventbuilderbinary.dir/build: lib/libeventbuilderbinary.so

.PHONY : daqling/src/Modules/CMakeFiles/eventbuilderbinary.dir/build

daqling/src/Modules/CMakeFiles/eventbuilderbinary.dir/clean:
	cd /home/engamber/workspace/daqling_top/build/daqling/src/Modules && $(CMAKE_COMMAND) -P CMakeFiles/eventbuilderbinary.dir/cmake_clean.cmake
.PHONY : daqling/src/Modules/CMakeFiles/eventbuilderbinary.dir/clean

daqling/src/Modules/CMakeFiles/eventbuilderbinary.dir/depend:
	cd /home/engamber/workspace/daqling_top/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/engamber/workspace/daqling_top /home/engamber/workspace/daqling_top/daqling/src/Modules /home/engamber/workspace/daqling_top/build /home/engamber/workspace/daqling_top/build/daqling/src/Modules /home/engamber/workspace/daqling_top/build/daqling/src/Modules/CMakeFiles/eventbuilderbinary.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : daqling/src/Modules/CMakeFiles/eventbuilderbinary.dir/depend
