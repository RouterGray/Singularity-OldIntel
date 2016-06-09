# -*- cmake -*-

# these should be moved to their own cmake file
include(Prebuilt)
include(Boost)
include(Colladadom)


use_prebuilt_binary(libxml2)

set(LLPRIMITIVE_INCLUDE_DIRS
  ${LIBS_OPEN_DIR}/llprimitive
  ${COLLADADOM_INCLUDE_DIRS}
  )

if (WINDOWS)
    set(LLPRIMITIVE_LIBRARIES
        llprimitive
        ${COLLADADOM_LIBRARIES}
        libxml2_a
        )
else (WINDOWS)
    set(LLPRIMITIVE_LIBRARIES 
        llprimitive
        ${COLLADADOM_LIBRARIES}
        minizip
        xml2
        )
endif (WINDOWS)

