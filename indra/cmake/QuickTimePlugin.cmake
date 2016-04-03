# -*- cmake -*-

if(INSTALL_PROPRIETARY OR USE_QUICKTIME_PREBUILT)
  include(Prebuilt)
  if (WINDOWS)
    use_prebuilt_binary(quicktime)
  endif (WINDOWS)
endif(INSTALL_PROPRIETARY OR USE_QUICKTIME_PREBUILT)

if (DARWIN)
  include(CMakeFindFrameworks)
  find_library(QUICKTIME_LIBRARY QuickTime)
elseif (WINDOWS AND WORD_SIZE EQUAL 32)
  SET(program_files "ProgramFiles(x86)")
  SET(program_files $ENV{${program_files}})
  if(NOT program_files)
	SET(program_files $ENV{ProgramW6432})
  endif(NOT program_files)
  if(NOT program_files)
    SET(program_files $ENV{ProgramFiles})
  endif(NOT program_files)

  set(QUICKTIME_SDK_DIR "${program_files}/QuickTime SDK"
      CACHE PATH "Location of the QuickTime SDK.")

  find_library(DEBUG_QUICKTIME_LIBRARY qtmlclient
               PATHS
               ${ARCH_PREBUILT_DIRS_DEBUG}
               "${QUICKTIME_SDK_DIR}\\libraries"
               )

  find_library(RELEASE_QUICKTIME_LIBRARY qtmlclient
               PATHS
               ${ARCH_PREBUILT_DIRS_RELEASE}
               "${QUICKTIME_SDK_DIR}\\libraries"
               )

  if (DEBUG_QUICKTIME_LIBRARY AND RELEASE_QUICKTIME_LIBRARY)
    set(QUICKTIME_LIBRARY 
        optimized ${RELEASE_QUICKTIME_LIBRARY}
        debug ${DEBUG_QUICKTIME_LIBRARY}
        )
        
  endif (DEBUG_QUICKTIME_LIBRARY AND RELEASE_QUICKTIME_LIBRARY)
  
  include_directories(
    ${LIBS_PREBUILT_DIR}/include/quicktime
    "${QUICKTIME_SDK_DIR}\\CIncludes"
    )
endif (DARWIN)

mark_as_advanced(QUICKTIME_LIBRARY)

if (QUICKTIME_LIBRARY)
  set(QUICKTIME ON CACHE BOOL "Build with QuickTime streaming media support.")
endif (QUICKTIME_LIBRARY)

