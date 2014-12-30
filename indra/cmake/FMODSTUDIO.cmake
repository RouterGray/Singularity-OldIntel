# -*- cmake -*-

include(Linking)

if (FMODEX AND FMODSTUDIO)
    message( FATAL_ERROR "You can not enable two FMOD variants at the same time." )
endif (FMODEX AND FMODSTUDIO)

unset(FMOD_LIBRARY_RELEASE CACHE)
unset(FMOD_LIBRARY_DEBUG CACHE)
unset(FMOD_INCLUDE_DIR CACHE)

if (NOT FMODSTUDIO_SDK_DIR AND WINDOWS)
  GET_FILENAME_COMPONENT(REG_DIR [HKEY_CURRENT_USER\\Software\\FMOD\ Studio\ API\ Windows] ABSOLUTE)
  set(FMODSTUDIO_SDK_DIR ${REG_DIR} CACHE PATH "Path to the FMOD Studio SDK." FORCE)
endif (NOT FMODSTUDIO_SDK_DIR AND WINDOWS)

set(release_fmod_lib_paths
    ${LIBS_PREBUILT_DIR}/release/lib/
    ${LIBS_PREBUILT_LEGACY_DIR}/release/lib)
set(debug_fmod_lib_paths
    ${LIBS_PREBUILT_DIR}/debug/lib
    ${LIBS_PREBUILT_LEGACY_DIR}/debug/lib)
set(fmod_inc_paths
    ${LIBS_PREBUILT_DIR}/include/fmodstudio
    ${LIBS_PREBUILT_LEGACY_DIR}/include/fmodstudio)



if (FMODSTUDIO_SDK_DIR)
  if(LINUX AND WORD_SIZE EQUAL 32)
    set(release_lib_paths ${release_fmod_lib_paths} "${FMODSTUDIO_SDK_DIR}/api/lowlevel/x86/lib" )
    set(debug__lib_paths ${debug_fmod_lib_paths} "${FMODSTUDIO_SDK_DIR}/api/lowlevel/x86/lib")
  elseif(LINUX)
    set(release__lib_paths ${release_fmod_lib_paths} "${FMODSTUDIO_SDK_DIR}/api/lowlevel/x86_64/lib")
    set(debug_fmod_lib_paths ${debug_fmod_lib_paths} "${FMODSTUDIO_SDK_DIR}/api/lowlevel/x86_64/lib")
  else(LINUX AND WORD_SIZE EQUAL 32)
    set(release_fmod_lib_paths ${release_fmod_lib_paths} "${FMODSTUDIO_SDK_DIR}/api/lowlevel/lib")
    set(debug_fmod_lib_paths ${debug_fmod_lib_paths} "${FMODSTUDIO_SDK_DIR}/api/lowlevel/lib")
  endif(LINUX AND WORD_SIZE EQUAL 32)
  set(fmod_inc_paths ${fmod_inc_paths} "${FMODSTUDIO_SDK_DIR}/api/lowlevel/inc")
endif (FMODSTUDIO_SDK_DIR)

if(WINDOWS)
  set(CMAKE_FIND_LIBRARY_SUFFIXES_OLD ${CMAKE_FIND_LIBRARY_SUFFIXES})
  set(CMAKE_FIND_LIBRARY_SUFFIXES .dll)
endif(WINDOWS)
if(WORD_SIZE EQUAL 32) #Check if CMAKE_FIND_LIBRARY_PREFIXES is set to 'lib' for darwin.
  find_library(FMOD_LIBRARY_RELEASE fmod PATHS ${release_fmod_lib_paths})
  find_library(FMOD_LIBRARY_DEBUG fmodL PATHS ${debug_fmod_lib_paths})
elseif(WORD_SIZE EQUAL 64)
  find_library(FMOD_LIBRARY_RELEASE fmod64 PATHS ${release_fmod_lib_paths})
  find_library(FMOD_LIBRARY_DEBUG fmodL64 PATHS ${debug_fmod_lib_paths}) 
endif (WORD_SIZE EQUAL 32)
if(WINDOWS)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_OLD})
  string(REPLACE ".dll" "_vc.lib" FMOD_LINK_LIBRARY_RELEASE ${FMOD_LIBRARY_RELEASE})
  string(REPLACE ".dll" "_vc.lib" FMOD_LINK_LIBRARY_DEBUG ${FMOD_LIBRARY_DEBUG})
else(WINDOWS)
  set(FMODSTUDIO_LINK_LIBRARY_RELEASE  ${FMODSTUDIO_LIBRARY_RELEASE})
  set(FMODSTUDIO_LINK_LIBRARY_DEBUG ${FMODSTUDIO_LIBRARY_DEBUG})
endif(WINDOWS)
find_path(FMOD_INCLUDE_DIR fmod.hpp ${fmod_inc_paths})

if (FMOD_LIBRARY_RELEASE AND FMOD_INCLUDE_DIR)
  set(FMOD ON)
  if (NOT FMOD_LIBRARY_DEBUG) #Use release library in debug configuration if debug library is absent.
    set(FMOD_LIBRARY_DEBUG ${FMOD_LIBRARY_RELEASE})
  endif (NOT FMOD_LIBRARY_DEBUG)
else (FMOD_LIBRARY_RELEASE AND FMOD_INCLUDE_DIR)
  message(STATUS "No support for FMOD Studio audio (need to set FMODSTUDIO_SDK_DIR?)")
  set(FMOD OFF)
  set(FMODSTUDIO OFF)
endif (FMOD_LIBRARY_RELEASE AND FMOD_INCLUDE_DIR)

if (FMOD)
  message(STATUS "Building with FMOD Studio audio support")
endif (FMOD)
