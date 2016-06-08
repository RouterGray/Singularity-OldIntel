# -*- cmake -*-

# cmake_minimum_required should appear before any
# other commands to guarantee full compatibility
# with the version specified
## prior to 2.8, the add_custom_target commands used in setting the version did not work correctly
if(WIN32)
  cmake_minimum_required(VERSION 3.4 FATAL_ERROR)
else()
  cmake_minimum_required(VERSION 2.8.8 FATAL_ERROR)
endif()

set(ROOT_PROJECT_NAME "Singularity" CACHE STRING
    "The root project/makefile/solution name. Defaults to Singularity.")
project(${ROOT_PROJECT_NAME})

string(REPLACE " " "" VIEWER_CHANNEL_NOSPACE ${VIEWER_CHANNEL})
set(VIEWER_CHANNEL_NOSPACE ${VIEWER_CHANNEL_NOSPACE} CACHE STRING "Prefix used for resulting artifacts.")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(Variables)
include(BuildVersion)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Build type.  One of: Debug Release RelWithDebInfo" FORCE)
endif (NOT CMAKE_BUILD_TYPE)

add_subdirectory(cmake)
add_subdirectory(${LIBS_OPEN_PREFIX}llaudio)
add_subdirectory(${LIBS_OPEN_PREFIX}llappearance)
add_subdirectory(${LIBS_OPEN_PREFIX}llcharacter)
add_subdirectory(${LIBS_OPEN_PREFIX}llcommon)
add_subdirectory(${LIBS_OPEN_PREFIX}llcorehttp)
add_subdirectory(${LIBS_OPEN_PREFIX}llimage)
add_subdirectory(${LIBS_OPEN_PREFIX}libopenjpeg)
add_subdirectory(${LIBS_OPEN_PREFIX}libhacd)
add_subdirectory(${LIBS_OPEN_PREFIX}libndhacd)
add_subdirectory(${LIBS_OPEN_PREFIX}libpathing)
add_subdirectory(${LIBS_OPEN_PREFIX}llimagej2coj)
add_subdirectory(${LIBS_OPEN_PREFIX}llinventory)
add_subdirectory(${LIBS_OPEN_PREFIX}llmath)
add_subdirectory(${LIBS_OPEN_PREFIX}llmessage)
add_subdirectory(${LIBS_OPEN_PREFIX}llprimitive)
add_subdirectory(${LIBS_OPEN_PREFIX}llrender)
add_subdirectory(${LIBS_OPEN_PREFIX}llvfs)
add_subdirectory(${LIBS_OPEN_PREFIX}llwindow)
add_subdirectory(${LIBS_OPEN_PREFIX}llxml)

if (WINDOWS AND EXISTS ${LIBS_CLOSED_DIR}copy_win_scripts)
  add_subdirectory(${LIBS_CLOSED_PREFIX}copy_win_scripts)
endif (WINDOWS AND EXISTS ${LIBS_CLOSED_DIR}copy_win_scripts)

add_custom_target(viewer)
add_subdirectory(${LIBS_OPEN_PREFIX}llplugin)
add_subdirectory(${LIBS_OPEN_PREFIX}llui)

# viewer plugins directory
add_subdirectory(${LIBS_OPEN_PREFIX}media_plugins)

add_subdirectory(${VIEWER_PREFIX}newview)
add_dependencies(viewer singularity-bin)

