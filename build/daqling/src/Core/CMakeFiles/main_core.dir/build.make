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
include daqling/src/Core/CMakeFiles/main_core.dir/depend.make

# Include the progress variables for this target.
include daqling/src/Core/CMakeFiles/main_core.dir/progress.make

# Include the compile flags for this target's objects.
include daqling/src/Core/CMakeFiles/main_core.dir/flags.make

daqling/src/Core/CMakeFiles/main_core.dir/main_core.cpp.o: daqling/src/Core/CMakeFiles/main_core.dir/flags.make
daqling/src/Core/CMakeFiles/main_core.dir/main_core.cpp.o: ../daqling/src/Core/main_core.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/engamber/workspace/daqling_top/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object daqling/src/Core/CMakeFiles/main_core.dir/main_core.cpp.o"
	cd /home/engamber/workspace/daqling_top/build/daqling/src/Core && /cvmfs/sft.cern.ch/lcg/releases/gcc/6.2.0-b9934/x86_64-centos7/bin/g++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/main_core.dir/main_core.cpp.o -c /home/engamber/workspace/daqling_top/daqling/src/Core/main_core.cpp

daqling/src/Core/CMakeFiles/main_core.dir/main_core.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/main_core.dir/main_core.cpp.i"
	cd /home/engamber/workspace/daqling_top/build/daqling/src/Core && /cvmfs/sft.cern.ch/lcg/releases/gcc/6.2.0-b9934/x86_64-centos7/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/engamber/workspace/daqling_top/daqling/src/Core/main_core.cpp > CMakeFiles/main_core.dir/main_core.cpp.i

daqling/src/Core/CMakeFiles/main_core.dir/main_core.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/main_core.dir/main_core.cpp.s"
	cd /home/engamber/workspace/daqling_top/build/daqling/src/Core && /cvmfs/sft.cern.ch/lcg/releases/gcc/6.2.0-b9934/x86_64-centos7/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/engamber/workspace/daqling_top/daqling/src/Core/main_core.cpp -o CMakeFiles/main_core.dir/main_core.cpp.s

# Object files for target main_core
main_core_OBJECTS = \
"CMakeFiles/main_core.dir/main_core.cpp.o"

# External object files for target main_core
main_core_EXTERNAL_OBJECTS =

bin/main_core: daqling/src/Core/CMakeFiles/main_core.dir/main_core.cpp.o
bin/main_core: daqling/src/Core/CMakeFiles/main_core.dir/build.make
bin/main_core: lib/libcore.so
bin/main_core: lib/libutilities.so
bin/main_core: daqling/src/Core/CMakeFiles/main_core.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/engamber/workspace/daqling_top/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../../../bin/main_core"
	cd /home/engamber/workspace/daqling_top/build/daqling/src/Core && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/main_core.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
daqling/src/Core/CMakeFiles/main_core.dir/build: bin/main_core

.PHONY : daqling/src/Core/CMakeFiles/main_core.dir/build

daqling/src/Core/CMakeFiles/main_core.dir/clean:
	cd /home/engamber/workspace/daqling_top/build/daqling/src/Core && $(CMAKE_COMMAND) -P CMakeFiles/main_core.dir/cmake_clean.cmake
.PHONY : daqling/src/Core/CMakeFiles/main_core.dir/clean

daqling/src/Core/CMakeFiles/main_core.dir/depend:
	cd /home/engamber/workspace/daqling_top/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/engamber/workspace/daqling_top /home/engamber/workspace/daqling_top/daqling/src/Core /home/engamber/workspace/daqling_top/build /home/engamber/workspace/daqling_top/build/daqling/src/Core /home/engamber/workspace/daqling_top/build/daqling/src/Core/CMakeFiles/main_core.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : daqling/src/Core/CMakeFiles/main_core.dir/depend

