# -*- cmake -*-

add_subdirectory(base_basic)
add_subdirectory(base_media)
add_subdirectory(filepicker)
if (NOT LINUX)
add_subdirectory(cef)
endif (NOT LINUX)

if (LINUX)
    add_subdirectory(gstreamer010)
endif (LINUX)

if (WINDOWS OR DARWIN)
    add_subdirectory(quicktime)
endif (WINDOWS OR DARWIN)

add_subdirectory(example_basic)
add_subdirectory(example_media)
