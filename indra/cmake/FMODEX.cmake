# -*- cmake -*-

include(Linking)

if (FMODEX AND FMODSTUDIO)
    message( FATAL_ERROR "You can not enable two FMOD variants at the same time." )
endif (FMODEX AND FMODSTUDIO)

unset(FMOD_LIBRARY_RELEASE CACHE)
unset(FMOD_LIBRARY_DEBUG CACHE)
unset(FMOD_INCLUDE_DIR CACHE)

if (NOT FMODEX_SDK_DIR AND WINDOWS)
  GET_FILENAME_COMPONENT(REG_DIR [HKEY_CURRENT_USER\\Software\\FMOD\ Programmers\ API\ Windows] ABSOLUTE)
  set(FMODEX_SDK_DIR ${REG_DIR} CACHE PATH "Path to the FMOD Ex SDK." FORCE)
endif (NOT FMODEX_SDK_DIR AND WINDOWS)

set(release_fmod_lib_paths
    ${LIBS_PREBUILT_DIR}/release/lib/
    ${LIBS_PREBUILT_LEGACY_DIR}/release/lib)
set(debug_fmod_lib_paths
    ${LIBS_PREBUILT_DIR}/debug/lib
    ${LIBS_PREBUILT_LEGACY_DIR}/debug/lib)
set(fmod_inc_paths
    ${LIBS_PREBUILT_DIR}/include/fmodex
    ${LIBS_PREBUILT_LEGACY_DIR}/include/fmodex)

if (FMODEX_SDK_DIR)
  set(release_fmod_lib_paths ${release_fmod_lib_paths} "${FMODEX_SDK_DIR}/api" "${FMODEX_SDK_DIR}/api/lib")
  set(debug_fmod_lib_paths ${debug_fmod_lib_paths} "${FMODEX_SDK_DIR}/api" "${FMODEX_SDK_DIR}/api/lib")
  set(fmod_inc_paths ${fmod_inc_paths} "${FMODEX_SDK_DIR}/api/inc")
endif (FMODEX_SDK_DIR)

if(WINDOWS)
  set(CMAKE_FIND_LIBRARY_SUFFIXES_OLD ${CMAKE_FIND_LIBRARY_SUFFIXES})
  set(CMAKE_FIND_LIBRARY_SUFFIXES .dll)
endif(WINDOWS)
if(WORD_SIZE EQUAL 32) #Check if CMAKE_FIND_LIBRARY_PREFIXES is set to 'lib' for darwin.
  find_library(FMOD_LIBRARY_RELEASE fmodex PATHS ${release_fmod_lib_paths})
  find_library(FMOD_LIBRARY_DEBUG fmodexL PATHS ${debug_fmod_lib_paths})
elseif(WORD_SIZE EQUAL 64)
  find_library(FMOD_LIBRARY_RELEASE fmodex64 PATHS ${release_fmod_lib_paths})
  find_library(FMOD_LIBRARY_DEBUG fmodLex64 PATHS ${debug_fmod_lib_paths}) 
endif (WORD_SIZE EQUAL 32)
if(WINDOWS)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_OLD})
  if(WORD_SIZE EQUAL 32)
    find_library(FMOD_LINK_LIBRARY_RELEASE fmodex_vc PATHS ${release_fmod_lib_paths})
    find_library(FMOD_LINK_LIBRARY_DEBUG fmodexL_vc PATHS ${debug_fmod_lib_paths})
  elseif(WORD_SIZE EQUAL 64)
    find_library(FMOD_LINK_LIBRARY_RELEASE fmodex64_vc PATHS ${release_fmod_lib_paths})
    find_library(FMOD_LINK_LIBRARY_DEBUG fmodLex64_vc PATHS ${debug_fmod_lib_paths}) 
  endif (WORD_SIZE EQUAL 32)
else(WINDOWS)
  set(FMOD_LINK_LIBRARY_RELEASE  ${FMODSTUDIO_LIBRARY_RELEASE})
  set(FMOD_LINK_LIBRARY_DEBUG ${FMODSTUDIO_LIBRARY_DEBUG})
endif(WINDOWS)
find_path(FMOD_INCLUDE_DIR fmod.hpp ${fmod_inc_paths})

if (FMOD_LIBRARY_RELEASE AND FMOD_INCLUDE_DIR)
  set(FMOD ON)
  if (NOT FMOD_LIBRARY_DEBUG) #Use release library in debug configuration if debug library is absent.
    set(FMOD_LIBRARY_DEBUG ${FMOD_LIBRARY_RELEASE})
  endif (NOT FMOD_LIBRARY_DEBUG)
else (FMOD_LIBRARY_RELEASE AND FMOD_INCLUDE_DIR)
  message(STATUS "No support for FMOD Ex audio (need to set FMODEX_SDK_DIR?)")
  set(FMOD OFF)
  set(FMODEX OFF)
endif (FMOD_LIBRARY_RELEASE AND FMOD_INCLUDE_DIR)

if (FMOD)
  message(STATUS "Building with FMOD Ex audio support")
endif (FMOD)
