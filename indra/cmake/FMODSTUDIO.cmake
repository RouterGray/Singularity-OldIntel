# -*- cmake -*-

include(Linking)

if (FMODEX AND FMODSTUDIO)
    message( FATAL_ERROR "You can not enable two FMOD variants at the same time." )
endif (FMODEX AND FMODSTUDIO)

if (NOT FMODSTUDIO_LIBRARY)
  set(FMODSTUDIO_SDK_DIR CACHE PATH "Path to the FMOD Ex SDK.")
  if (FMODSTUDIO_SDK_DIR)
    if(WORD_SIZE EQUAL 32)
      find_library(FMODSTUDIO_LIBRARY
                   fmod_vc fmodL_vc fmod fmodL
                   PATHS
                   "${FMODSTUDIO_SDK_DIR}/api/lowlevel/lib"
                   "${FMODSTUDIO_SDK_DIR}/api/lowlevel"
                   "${FMODSTUDIO_SDK_DIR}"
                   )
    elseif(WORD_SIZE EQUAL 64)
      find_library(FMODSTUDIO_LIBRARY
                   fmod64_vc fmodL64_vc fmod64 fmodL64
                   PATHS
                   "${FMODSTUDIO_SDK_DIR}/api/lowlevel/lib"
                   "${FMODSTUDIO_SDK_DIR}/api/lowlevel"
                   "${FMODSTUDIO_SDK_DIR}"
                   )
    endif(WORD_SIZE EQUAL 32)
  endif(FMODSTUDIO_SDK_DIR)
  if(WINDOWS AND NOT FMODSTUDIO_SDK_DIR)
    GET_FILENAME_COMPONENT(FMODSTUDIO_PROG_DIR [HKEY_CURRENT_USER\\Software\\FMOD\ Studio\ API\ Windows] ABSOLUTE CACHE)
    if(WORD_SIZE EQUAL 32)
      find_library(FMODSTUDIO_LIBRARY
                   fmod_vc fmodL_vc
                   PATHS
                   "${FMODSTUDIO_PROG_DIR}/api/lowlevel/lib"
                   "${FMODSTUDIO_PROG_DIR}/api/lowlevel"
                   "${FMODSTUDIO_PROG_DIR}"
                   )
    else(WORD_SIZE EQUAL 32)
      find_library(FMODSTUDIO_LIBRARY
                   fmod64_vc fmodL64_vc
                   PATHS
                   "${FMODSTUDIO_PROG_DIR}/api/lowlevel/lib"
                   "${FMODSTUDIO_PROG_DIR}/api/lowlevel"
                   "${FMODSTUDIO_PROG_DIR}"
                   )
    endif(WORD_SIZE EQUAL 32)
    if(FMODSTUDIO_LIBRARY)
      message(STATUS "Found fmodstudio in ${FMODSTUDIO_PROG_DIR}")
      set(FMODSTUDIO_SDK_DIR "${FMODSTUDIO_PROG_DIR}")
      set(FMODSTUDIO_SDK_DIR "${FMODSTUDIO_PROG_DIR}" CACHE PATH "Path to the FMOD Studio SDK." FORCE)
    endif(FMODSTUDIO_LIBRARY)
  endif(WINDOWS AND NOT FMODSTUDIO_SDK_DIR)
endif (NOT FMODSTUDIO_LIBRARY)

find_path(FMODSTUDIO_INCLUDE_DIR fmod.hpp
          ${LIBS_PREBUILT_DIR}/include/fmodstudio
          ${LIBS_PREBUILT_LEGACY_DIR}/include/fmodstudio
          "${FMODSTUDIO_SDK_DIR}/api/lowlevel/inc"
          "${FMODSTUDIO_SDK_DIR}"
          )

if(DARWIN)
  set(FMODSTUDIO_ORIG_LIBRARY "${FMODSTUDIO_LIBRARY}")
  set(FMODSTUDIO_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/libfmod.dylib")
endif(DARWIN)

if (FMODSTUDIO_LIBRARY AND FMODSTUDIO_INCLUDE_DIR)
  set(FMODSTUDIO ON CACHE BOOL "Use closed source FMOD Studio sound library.")
else (FMODSTUDIO_LIBRARY AND FMODSTUDIO_INCLUDE_DIR)
  set(FMODSTUDIO_LIBRARY "")
  set(FMODSTUDIO_INCLUDE_DIR "")
  if (FMODSTUDIO)
    message(STATUS "No support for FMOD Studio audio (need to set FMODSTUDIO_SDK_DIR?)")
  endif (FMODSTUDIO)
  set(FMODSTUDIO OFF CACHE BOOL "Use closed source FMOD Studio sound library.")
  set(FMODSTUDIO OFF)
endif (FMODSTUDIO_LIBRARY AND FMODSTUDIO_INCLUDE_DIR)

if (FMODSTUDIO)
  message(STATUS "Building with FMOD Studio audio support")
endif (FMODSTUDIO)
