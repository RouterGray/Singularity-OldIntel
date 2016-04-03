# -*- cmake -*-
include(Prebuilt)

set(CURL_FIND_QUIETLY OFF)
set(CURL_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindCURL)
else (STANDALONE)
  use_prebuilt_binary(curl)
  if (WINDOWS)
    set(CURL_LIBRARIES 
    debug libcurld
    optimized libcurl)
  else (WINDOWS)
    use_prebuilt_binary(libidn)
    set(CURL_LIBRARIES curl idn)
  endif (WINDOWS)
  set(CURL_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (STANDALONE)
