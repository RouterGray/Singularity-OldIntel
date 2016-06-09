# -*- cmake -*-

include(CURL)
include(OpenSSL)
include(Boost)

set(LLCOREHTTP_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llcorehttp
    ${CURL_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    ${BOOST_INCLUDE_DIRS}
    )

set(LLCOREHTTP_LIBRARIES llcorehttp)
