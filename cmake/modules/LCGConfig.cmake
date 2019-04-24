# $Id: LCGConfig.cmake 153583 2015-10-15 17:31:19Z krasznaa $
#
# File implementing the code that gets called when a project imports
# LCG using something like:
#
#  find_package( LCG 75 )
#
# Hopefully this could be adopted by the SFT group in the official LCG
# builds, but for now it's only a demonstrator.
#
# To make CMake find this file, you have to call CMake either like:
#
#   CMAKE_PREFIX_PATH=../source/AtlasCMake cmake ../source
#
# , or like:
#
#   cmake -DLCG_DIR=../../AtlasCMake ../source
#
# The script takes the value of the environment variable (or CMake cache
# variable) LCG_RELEASES_BASE to find LCG releases. Or if it's not set, it
# looks up the releases from AFS.
#

# If LCG was already found, don't execute this code again:
if( LCG_FOUND )
   return()
endif()

# This function is used by the code to get the "compiler portion"
# of the platform name. E.g. for GCC 4.9.2, return "gcc49". In case
# the compiler and version are not understood, the functions returns
# a false value in its second argument.
#
# The decision is made based on the C++ compiler.
#
# Usage: lcg_compiler_id( _cmp _isValid )
#
function( lcg_compiler_id compiler isValid )

   # Translate the compiler ID:
   set( _prefix )
   if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
      set( _prefix "gcc" )
   elseif( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
      set( _prefix "clang" )
   elseif( CMAKE_CXX_COMPILER_ID STREQUAL "Intel" )
      set( _prefix "gcc" )
   else()
      set( ${compiler} "unknown" PARENT_SCOPE )
      set( ${isValid} FALSE PARENT_SCOPE )
      return()
   endif()

   # Translate the compiler version:
   set( _version )
   if( CMAKE_CXX_COMPILER_ID STREQUAL "Intel" )
      set( _version "62" )
   else()
      if( CMAKE_CXX_COMPILER_VERSION MATCHES "^([0-9]+).([0-9]+).*" )
         set( _version "${CMAKE_MATCH_1}${CMAKE_MATCH_2}" )
      elseif( CMAKE_CXX_COMPILER_VERSION MATCHES "^([0-9]+).*" )
         set( _version "${CMAKE_MATCH_1}" )
      endif()
   endif()

   # Set the return variables:
   set( ${compiler} "${_prefix}${_version}" PARENT_SCOPE )
   set( ${isValid} TRUE PARENT_SCOPE )

endfunction( lcg_compiler_id )

# This function is used to get a compact OS designation for the platform
# name. Like "slc6", "mac1010" or "cc7".
#
# Usage: lcg_os_id( _os _isValid )
#
function( lcg_os_id os isValid )

   # Reset the result variable as a start:
   set( _name )

   if( APPLE )
      # Get the MacOS X version number from the command line:
      execute_process( COMMAND sw_vers -productVersion
         TIMEOUT 2
         OUTPUT_VARIABLE _macVers )
      # Parse the variable, which should be in the form "X.Y.Z", or
      # possibly just "X.Y":
      if( _macVers MATCHES "^([0-9]+).([0-9]+).*" )
         set( _name "mac${CMAKE_MATCH_1}${CMAKE_MATCH_2}" )
      else()
         set( ${os} "unknown" PARENT_SCOPE )
         set( ${isValid} FALSE PARENT_SCOPE )
         return()
      endif()
   elseif( UNIX )
      # Get the linux release ID:
      execute_process( COMMAND lsb_release -i
         TIMEOUT 2
         OUTPUT_VARIABLE _linuxId )
      # Translate it to a shorthand according to our own naming:
      set( _linuxShort )
      if( _linuxId MATCHES "Scientific" )
         set( _linuxShort "slc" )
      elseif( _linuxId MATCHES "Ubuntu" )
         set( _linuxShort "ubuntu" )
      elseif( _linuxId MATCHES "CentOS" )
         set( _linuxShort "centos" )
      else()
         message( WARNING "Linux flavour not recognised" )
         set( _linuxShort "linux" )
      endif()
      # Get the linux version number:
      execute_process( COMMAND lsb_release -r
         TIMEOUT 2
         OUTPUT_VARIABLE _linuxVers )
      # Try to parse it:
      if( _linuxVers MATCHES "^Release:[^0-9]*([0-9]+)\\..*" )
         set( _name "${_linuxShort}${CMAKE_MATCH_1}" )
      else()
         set( ${os} "unknown" PARENT_SCOPE )
         set( ${isValid} FALSE PARENT_SCOPE )
         return()
      endif()
   else()
      set( ${os} "unknown" PARENT_SCOPE )
      set( ${isValid} FALSE PARENT_SCOPE )
      return()
   endif()

   # Set the return values:
   set( ${os} ${_name} PARENT_SCOPE )
   set( ${isValid} TRUE PARENT_SCOPE )

endfunction( lcg_os_id )

# This function is used internally to construct a platform name for a
# project. Something like: "x86_64-slc6-gcc48-opt".
#
# Usage: lcg_platform_id( _platform )
#
function( lcg_platform_id platform )

   # Get the OS's name:
   lcg_os_id( _os _valid )
   if( NOT _valid )
      set( ${platform} "generic" PARENT_SCOPE )
      return()
   endif()

   # Get the compiler name:
   lcg_compiler_id( _cmp _valid )
   if( NOT _valid )
      set( ${platform} "generic" PARENT_SCOPE )
      return()
   endif()

   # Construct the postfix of the platform name:
   if( CMAKE_BUILD_TYPE STREQUAL "Debug" )
      set( _postfix "dbg" )
   else()
      set( _postfix "opt" )
   endif()

   # Set the platform return value:
   set( ${platform} "${CMAKE_SYSTEM_PROCESSOR}-${_os}-${_cmp}-${_postfix}"
      PARENT_SCOPE )

endfunction( lcg_platform_id )

# Get the platform ID:
lcg_platform_id( LCG_PLATFORM )

# Tell the user what's happening:
message( STATUS
   "Setting up LCG release \"${LCG_VERSION}\" for platform: ${LCG_PLATFORM}" )

# Some sanity checks:
if( LCG_FIND_COMPONENTS )
   message( WARNING "Components \"${LCG_FIND_COMPONENTS}\" requested, but "
      "finding LCG components is not supported" )
endif()

# Construct the path to pick up the release from:
set( LCG_RELEASE_DIR ${LCG_RELEASE_BASE}/LCG_${LCG_VERSION} )

# The components to set up:
set( LCG_COMPONENTS externals)

# Start out with the assumption that LCG is now found:
set( LCG_FOUND TRUE )

function(lcg_setup_components)

  # Set up the packages provided by LCG using the files specified:
  foreach( _component ${LCG_COMPONENTS} )

    # Construct the file name to load:
    set( _file ${LCG_RELEASE_DIR}/LCG_${_component}_${LCG_PLATFORM}.txt )
    # set( _file ${LCG_RELEASE_DIR}/LCG_${_component}_x86_64-slc6-gcc62-opt.txt )
    if( NOT EXISTS ${_file} )
      message( SEND_ERROR
        "LCG component \"${_component}\" not available for platform: "
        "${LCG_PLATFORM}" )
      set( LCG_FOUND FALSE PARENT_SCOPE)
      continue()
    endif()
    
    # Tell the user what's happening:
    message( STATUS "Setting up LCG release using: ${_file}" )
    
    # Read in the contents of the file:
    file( STRINGS ${_file} _fileContents REGEX ".*;.*;.*;.*;.*" )
    
    # Loop over each line of the configuration file:
    foreach( _line ${_fileContents} )

      # The component's name:
      list( GET _line 0 name )
      string( TOUPPER ${name} nameUpper )
      # The component's identifier:
      list( GET _line 1 id )
      # The component's version:
      list( GET _line 2 version )
      string(STRIP ${version} version)
      # The component's base directory:
      list( GET _line 3 dir1 )
      string( STRIP ${dir1} dir2 )

      # Handle the case where the directory is an absolute path
      # e.g. for LCG nightlies ?
      if(${dir2} MATCHES "^/.*")
        set(dir3 ${dir2})
      else()
        string( REPLACE "./" "" dir3 ${dir2} )
        set(dir3 ${LCG_RELEASE_DIR}/${dir3})
      endif()
      # The component's dependencies:
      list( GET _line 4 dep )

      if(${name} STREQUAL "CMake")
         continue()
      endif()
      
      # Set up the component. In an extremely simple way for now, which
      # just assumes that the Find<Component>.cmake files will find
      # these components based on the <Component>_ROOT or possibly
      # <Component>_DIR variable.
      set( ${nameUpper}_ROOT ${dir3}
        CACHE PATH "Directory for ${name}-${version}" FORCE )
      set( ${nameUpper}_DIR ${dir3}
        CACHE PATH "Directory for ${name}-${version}" FORCE )
      set( ${nameUpper}_ROOT_DIR ${dir3}
        CACHE PATH "Directory for ${name}-${version}" FORCE )
      set( ${nameUpper}_VERSION ${version} PARENT_SCOPE)
    endforeach()
  endforeach()

endfunction()

lcg_setup_components()

#
# Extra setting(s) for some package(s):
#
file( GLOB BOOST_INCLUDEDIR "${BOOST_ROOT}/include/*" )

# needed for PythonInterp
set(CMAKE_PROGRAM_PATH "${PYTHON_ROOT}/bin")
# needed for PythonLibs
file(GLOB PYTHON_INCLUDE_DIR ${PYTHON_ROOT}/include/python*)
set(PYTHON_INCLUDE_DIR ${PYTHON_INCLUDE_DIR} CACHE PATH "Python include directory" FORCE)
file(GLOB PYTHON_LIBRARY ${PYTHON_ROOT}/lib/libpython?.?.so)
set(PYTHON_LIBRARY ${PYTHON_LIBRARY} CACHE PATH "Python library" FORCE)

set(pytools_home ${PYTOOLS_ROOT})

# needed ROOT
set(ROOTSYS "${ROOT_ROOT}" CACHE PATH "ROOT directory" FORCE)

# needed for Qt4
set(ENV{QTDIR} ${QT_DIR})
set(QT_SEARCH_PATH ${QT_DIR} CACHE PATH "Qt4 directory" FORCE)


# Get the current directory:
get_filename_component( _thisdir "${CMAKE_CURRENT_LIST_FILE}" PATH )

# Add the module directory to CMake's module path:
list( APPEND CMAKE_MODULE_PATH ${_thisdir}/modules )
list( REMOVE_DUPLICATES CMAKE_MODULE_PATH )

# Extra build options needed by LCG packages:
set( CMAKE_CXX_STANDARD 14 CACHE STRING "C++ standard used for the build" )
set( CMAKE_CXX_EXTENSIONS FALSE CACHE BOOL "(Dis)allow using GNU extensions" )
