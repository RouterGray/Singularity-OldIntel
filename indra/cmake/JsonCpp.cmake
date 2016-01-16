# -*- cmake -*-

include(Prebuilt)

set(JSONCPP_FIND_QUIETLY OFF)
set(JSONCPP_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindJsonCpp)
else (STANDALONE)
  use_prebuilt_binary(jsoncpp)
  if (WINDOWS)
    set(JSONCPP_LIBRARIES
      debug jsoncppd.lib
      optimized jsoncpp.lib)
  elseif (DARWIN)
    set(JSONCPP_LIBRARIES json_linux-gcc-4.0.1_libmt)
  elseif (LINUX)
    set(JSONCPP_LIBRARIES jsoncpp)
  endif (WINDOWS)
  set(JSONCPP_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/jsoncpp)
endif (STANDALONE)
