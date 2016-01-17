# -*- cmake -*-
#
# Definitions of variables used throughout the Second Life build
# process.
#
# Platform variables:
#
#   DARWIN  - Mac OS X
#   LINUX   - Linux
#   WINDOWS - Windows


# Relative and absolute paths to subtrees.

if(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
set(${CMAKE_CURRENT_LIST_FILE}_INCLUDED "YES")

if(NOT DEFINED COMMON_CMAKE_DIR)
    set(COMMON_CMAKE_DIR "${CMAKE_SOURCE_DIR}/cmake")
endif(NOT DEFINED COMMON_CMAKE_DIR)

set(LIBS_CLOSED_PREFIX)
set(LIBS_OPEN_PREFIX)
set(SCRIPTS_PREFIX ../scripts)
set(VIEWER_PREFIX)
set(INTEGRATION_TESTS_PREFIX)
set(LL_TESTS OFF CACHE BOOL "Build and run unit and integration tests (disable for build timing runs to reduce variation)")

# Compiler and toolchain options
set(DISABLE_TCMALLOC OFF CACHE BOOL "Disable linkage of TCMalloc. (64bit builds automatically disable TCMalloc)")
set(DISABLE_FATAL_WARNINGS TRUE CACHE BOOL "Set this to FALSE to enable fatal warnings.")

if(LIBS_CLOSED_DIR)
  file(TO_CMAKE_PATH "${LIBS_CLOSED_DIR}" LIBS_CLOSED_DIR)
else(LIBS_CLOSED_DIR)
  set(LIBS_CLOSED_DIR ${CMAKE_SOURCE_DIR}/${LIBS_CLOSED_PREFIX})
endif(LIBS_CLOSED_DIR)
if(LIBS_COMMON_DIR)
  file(TO_CMAKE_PATH "${LIBS_COMMON_DIR}" LIBS_COMMON_DIR)
else(LIBS_COMMON_DIR)
  set(LIBS_COMMON_DIR ${CMAKE_SOURCE_DIR}/${LIBS_OPEN_PREFIX})
endif(LIBS_COMMON_DIR)
set(LIBS_OPEN_DIR ${LIBS_COMMON_DIR})

set(SCRIPTS_DIR ${CMAKE_SOURCE_DIR}/${SCRIPTS_PREFIX})
set(VIEWER_DIR ${CMAKE_SOURCE_DIR}/${VIEWER_PREFIX})

set(AUTOBUILD_INSTALL_DIR ${CMAKE_BINARY_DIR}/packages)

set(LIBS_PREBUILT_DIR ${AUTOBUILD_INSTALL_DIR} CACHE PATH
    "Location of prebuilt libraries.")

if (EXISTS ${CMAKE_SOURCE_DIR}/Server.cmake)
  # We use this as a marker that you can try to use the proprietary libraries.
  set(INSTALL_PROPRIETARY ON CACHE BOOL "Install proprietary binaries")
endif (EXISTS ${CMAKE_SOURCE_DIR}/Server.cmake)
set(TEMPLATE_VERIFIER_OPTIONS "" CACHE STRING "Options for scripts/template_verifier.py")
set(TEMPLATE_VERIFIER_MASTER_URL "http://bitbucket.org/lindenlab/master-message-template/raw/tip/message_template.msg" CACHE STRING "Location of the master message template")

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Build type.  One of: Debug Release RelWithDebInfo" FORCE)
endif (NOT CMAKE_BUILD_TYPE)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(WINDOWS ON BOOL FORCE)
  if (WORD_SIZE EQUAL 64)
    set(ARCH x86_64 CACHE STRING "Viewer Architecture")
    set(LL_ARCH ${ARCH}_win64)
    set(LL_ARCH_DIR ${ARCH}-win64)
    set(WORD_SIZE 64)
    set(AUTOBUILD_PLATFORM_NAME "windows64")
  else (WORD_SIZE EQUAL 64)
    set(ARCH i686 CACHE STRING "Viewer Architecture")
    set(LL_ARCH ${ARCH}_win32)
    set(LL_ARCH_DIR ${ARCH}-win32)
    set(WORD_SIZE 32)
    set(AUTOBUILD_PLATFORM_NAME "windows")
  endif (WORD_SIZE EQUAL 64)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(LINUX ON BOOl FORCE)

  # If someone has specified a word size, use that to determine the
  # architecture.  Otherwise, let the architecture specify the word size.
  if (WORD_SIZE EQUAL 32)
    set(ARCH i686)
  elseif (WORD_SIZE EQUAL 64)
    set(ARCH x86_64)
  else (WORD_SIZE EQUAL 32)
    if(CMAKE_SIZEOF_VOID_P MATCHES 4)
      set(ARCH i686)
      set(WORD_SIZE 32)
    else(CMAKE_SIZEOF_VOID_P MATCHES 4)
      set(ARCH x86_64)
      set(WORD_SIZE 64)
    endif(CMAKE_SIZEOF_VOID_P MATCHES 4)
  endif (WORD_SIZE EQUAL 32)

  if (NOT STANDALONE AND MULTIARCH_HACK)
    if (WORD_SIZE EQUAL 32)
      set(DEB_ARCHITECTURE i386)
      set(FIND_LIBRARY_USE_LIB64_PATHS OFF)
      set(CMAKE_SYSTEM_LIBRARY_PATH /usr/lib32 ${CMAKE_SYSTEM_LIBRARY_PATH})
    else (WORD_SIZE EQUAL 32)
      set(DEB_ARCHITECTURE amd64)
      set(FIND_LIBRARY_USE_LIB64_PATHS ON)
    endif (WORD_SIZE EQUAL 32)

    execute_process(COMMAND dpkg-architecture -a${DEB_ARCHITECTURE} -qDEB_HOST_MULTIARCH 
        RESULT_VARIABLE DPKG_RESULT
        OUTPUT_VARIABLE DPKG_ARCH
        OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    #message (STATUS "DPKG_RESULT ${DPKG_RESULT}, DPKG_ARCH ${DPKG_ARCH}")
    if (DPKG_RESULT EQUAL 0)
      set(CMAKE_LIBRARY_ARCHITECTURE ${DPKG_ARCH})
      set(CMAKE_SYSTEM_LIBRARY_PATH /usr/lib/${DPKG_ARCH} /usr/local/lib/${DPKG_ARCH} ${CMAKE_SYSTEM_LIBRARY_PATH})
    endif (DPKG_RESULT EQUAL 0)

    include(ConfigurePkgConfig)
  endif (NOT STANDALONE AND MULTIARCH_HACK)

  set(LL_ARCH ${ARCH}_linux)
  set(LL_ARCH_DIR ${ARCH}-linux)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(DARWIN 1)

  execute_process(
    COMMAND sh -c "xcodebuild -version | grep Xcode  | cut -d ' ' -f2 | cut -d'.' -f1-2"
    OUTPUT_VARIABLE XCODE_VERSION )
  string(REGEX REPLACE "(\r?\n)+$" "" XCODE_VERSION "${XCODE_VERSION}")

  # Hardcode SDK we build against until we can test and allow newer ones
  # as autodetected in the code above
  set(CMAKE_OSX_DEPLOYMENT_TARGET 10.6)
  set(CMAKE_OSX_SYSROOT macosx10.6)

  # Support for Unix Makefiles generator
  if (CMAKE_GENERATOR STREQUAL "Unix Makefiles")
    execute_process(COMMAND xcodebuild -version -sdk "${CMAKE_OSX_SYSROOT}" Path | head -n 1 OUTPUT_VARIABLE CMAKE_OSX_SYSROOT)
    string(REGEX REPLACE "(\r?\n)+$" "" CMAKE_OSX_SYSROOT "${CMAKE_OSX_SYSROOT}")
  endif (CMAKE_GENERATOR STREQUAL "Unix Makefiles")
      
  # LLVM-GCC has been removed in Xcode5
  if (XCODE_VERSION GREATER 4.9)
    set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")
  else (XCODE_VERSION GREATER 4.9)
    set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvmgcc42")
  endif (XCODE_VERSION GREATER 4.9)

  set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT dwarf-with-dsym)

  message(STATUS "Xcode version: ${XCODE_VERSION}")
  message(STATUS "OSX sysroot: ${CMAKE_OSX_SYSROOT}")
  message(STATUS "OSX deployment target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
  
  # Build only for i386 by default, system default on MacOSX 10.6 is x86_64
  set(CMAKE_OSX_ARCHITECTURES i386)
  set(ARCH i386)
  set(WORD_SIZE 32)

  set(LL_ARCH ${ARCH}_darwin)
  set(LL_ARCH_DIR universal-darwin)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

# Default deploy grid
set(GRID agni CACHE STRING "Target Grid")

set(VIEWER_CHANNEL "Singularity Test" CACHE STRING "Viewer Channel Name")

set(ENABLE_SIGNING OFF CACHE BOOL "Enable signing the viewer")
set(SIGNING_IDENTITY "" CACHE STRING "Specifies the signing identity to use, if necessary.")

set(VERSION_BUILD "0" CACHE STRING "Revision number passed in from the outside")
set(STANDALONE OFF CACHE BOOL "Do not use Linden-supplied prebuilt libraries.")

set(USE_PRECOMPILED_HEADERS ON CACHE BOOL "Enable use of precompiled header directives where supported.")

source_group("CMake Rules" FILES CMakeLists.txt)

endif(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
