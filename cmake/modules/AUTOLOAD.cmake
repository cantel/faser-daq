cmake_minimum_required(VERSION 3.4.3)

set(CMAKE_CXX_FLAGS -DGIT_DESCRIBE="`cd ${CMAKE_CURRENT_SOURCE_DIR}; git describe --tags --always --dirty`")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -O3 -g -pedantic")

if(NOT AUTOLOAD_INCLUDED)
  set(AUTOLOAD_INCLUDED TRUE)

  #
  # find externals correctly
  #
  set(VENDOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor)

  #
  # extra functions
  #
  function(add_external name version)
    message( STATUS "    External- " ${VENDOR_DIR} "/" ${name} " - " ${version})
    if(${ARGC} EQUAL 3)
      set(bintag ${ARGV2})
      #include_directories(${VENDOR_DIR}/${name}/${bintag}/include)
      #link_directories(${VENDOR_DIR}/${name}/${bintag}/lib)

      include_directories(${VENDOR_DIR}/${name}/include)
      link_directories(${VENDOR_DIR}/${name}/lib)
    else()
      include_directories(${VENDOR_DIR}/${name}/include)
    endif()
    add_subdirectory(${VENDOR_DIR}/${name})
  endfunction(add_external)

  function(library_version name version)
    message( STATUS "    Library " ${VENDOR_DIR} "/" ${name} " - " ${version})
  endfunction(library_version)

  #
  # Default settings
  #
  set(BUILD_SHARED_LIBS true)
  set(CMAKE_INCLUDE_CURRENT_DIR true)
  add_definitions(-D_GNU_SOURCE -D_REENTRANT -D__USE_XOPEN2K8)
  set(LCG_RELEASE_BASE $ENV{LCG_BASE}/releases)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/include)

  include(AUTOLOAD-versions)
endif()
