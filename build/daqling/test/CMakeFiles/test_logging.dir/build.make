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
include daqling/test/CMakeFiles/test_logging.dir/depend.make

# Include the progress variables for this target.
include daqling/test/CMakeFiles/test_logging.dir/progress.make

# Include the compile flags for this target's objects.
include daqling/test/CMakeFiles/test_logging.dir/flags.make

daqling/test/CMakeFiles/test_logging.dir/src/test_logging.cpp.o: daqling/test/CMakeFiles/test_logging.dir/flags.make
daqling/test/CMakeFiles/test_logging.dir/src/test_logging.cpp.o: ../daqling/test/src/test_logging.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/engamber/workspace/daqling_top/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object daqling/test/CMakeFiles/test_logging.dir/src/test_logging.cpp.o"
	cd /home/engamber/workspace/daqling_top/build/daqling/test && /cvmfs/sft.cern.ch/lcg/releases/gcc/6.2.0-b9934/x86_64-centos7/bin/g++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/test_logging.dir/src/test_logging.cpp.o -c /home/engamber/workspace/daqling_top/daqling/test/src/test_logging.cpp

daqling/test/CMakeFiles/test_logging.dir/src/test_logging.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/test_logging.dir/src/test_logging.cpp.i"
	cd /home/engamber/workspace/daqling_top/build/daqling/test && /cvmfs/sft.cern.ch/lcg/releases/gcc/6.2.0-b9934/x86_64-centos7/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/engamber/workspace/daqling_top/daqling/test/src/test_logging.cpp > CMakeFiles/test_logging.dir/src/test_logging.cpp.i

daqling/test/CMakeFiles/test_logging.dir/src/test_logging.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/test_logging.dir/src/test_logging.cpp.s"
	cd /home/engamber/workspace/daqling_top/build/daqling/test && /cvmfs/sft.cern.ch/lcg/releases/gcc/6.2.0-b9934/x86_64-centos7/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/engamber/workspace/daqling_top/daqling/test/src/test_logging.cpp -o CMakeFiles/test_logging.dir/src/test_logging.cpp.s

# Object files for target test_logging
test_logging_OBJECTS = \
"CMakeFiles/test_logging.dir/src/test_logging.cpp.o"

# External object files for target test_logging
test_logging_EXTERNAL_OBJECTS =

bin/test_logging: daqling/test/CMakeFiles/test_logging.dir/src/test_logging.cpp.o
bin/test_logging: daqling/test/CMakeFiles/test_logging.dir/build.make
bin/test_logging: lib/libcore.so
bin/test_logging: lib/libutilities.so
bin/test_logging: daqling/test/CMakeFiles/test_logging.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/engamber/workspace/daqling_top/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../../bin/test_logging"
	cd /home/engamber/workspace/daqling_top/build/daqling/test && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_logging.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
daqling/test/CMakeFiles/test_logging.dir/build: bin/test_logging

.PHONY : daqling/test/CMakeFiles/test_logging.dir/build

daqling/test/CMakeFiles/test_logging.dir/clean:
	cd /home/engamber/workspace/daqling_top/build/daqling/test && $(CMAKE_COMMAND) -P CMakeFiles/test_logging.dir/cmake_clean.cmake
.PHONY : daqling/test/CMakeFiles/test_logging.dir/clean

daqling/test/CMakeFiles/test_logging.dir/depend:
	cd /home/engamber/workspace/daqling_top/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/engamber/workspace/daqling_top /home/engamber/workspace/daqling_top/daqling/test /home/engamber/workspace/daqling_top/build /home/engamber/workspace/daqling_top/build/daqling/test /home/engamber/workspace/daqling_top/build/daqling/test/CMakeFiles/test_logging.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : daqling/test/CMakeFiles/test_logging.dir/depend

