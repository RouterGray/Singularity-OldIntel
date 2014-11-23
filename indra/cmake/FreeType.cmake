# -*- cmake -*-
include(Prebuilt)

if (STANDALONE)
  include(FindPkgConfig)

  pkg_check_modules(FREETYPE REQUIRED freetype2)
else (STANDALONE)
  use_prebuilt_binary(freetype)
  if (LINUX)
    set(FREETYPE_INCLUDE_DIRS
        ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/include)
  else (LINUX)
    if(MSVC12)
      set(FREETYPE_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/include/freetype2)
    else(MSVC12)
      set(FREETYPE_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/include)
	endif (MSVC12)
  endif (LINUX)

  set(FREETYPE_LIBRARIES freetype)
endif (STANDALONE)

link_directories(${FREETYPE_LIBRARY_DIRS})
