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

# Utility rule file for NightlyTest.

# Include the progress variables for this target.
include daqling/vendor/json/CMakeFiles/NightlyTest.dir/progress.make

daqling/vendor/json/CMakeFiles/NightlyTest:
	cd /home/engamber/workspace/daqling_top/build/daqling/vendor/json && /usr/bin/ctest3 -D NightlyTest

NightlyTest: daqling/vendor/json/CMakeFiles/NightlyTest
NightlyTest: daqling/vendor/json/CMakeFiles/NightlyTest.dir/build.make

.PHONY : NightlyTest

# Rule to build all files generated by this target.
daqling/vendor/json/CMakeFiles/NightlyTest.dir/build: NightlyTest

.PHONY : daqling/vendor/json/CMakeFiles/NightlyTest.dir/build

daqling/vendor/json/CMakeFiles/NightlyTest.dir/clean:
	cd /home/engamber/workspace/daqling_top/build/daqling/vendor/json && $(CMAKE_COMMAND) -P CMakeFiles/NightlyTest.dir/cmake_clean.cmake
.PHONY : daqling/vendor/json/CMakeFiles/NightlyTest.dir/clean

daqling/vendor/json/CMakeFiles/NightlyTest.dir/depend:
	cd /home/engamber/workspace/daqling_top/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/engamber/workspace/daqling_top /home/engamber/workspace/daqling_top/daqling/vendor/json /home/engamber/workspace/daqling_top/build /home/engamber/workspace/daqling_top/build/daqling/vendor/json /home/engamber/workspace/daqling_top/build/daqling/vendor/json/CMakeFiles/NightlyTest.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : daqling/vendor/json/CMakeFiles/NightlyTest.dir/depend

