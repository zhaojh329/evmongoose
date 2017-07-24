# The version number.
set(EMN_VERSION_MAJOR 0)
set(EMN_VERSION_MINOR 1)

find_package(Libev 4.22 REQUIRED)
find_package(HttpParser 2.1 REQUIRED)
find_package(OpenSSL)

set(EMN_SUPPORT_HTTPS 0)
set(SUPPORT_HTTPS_DEFAULT ON)

if (NOT OPENSSL_FOUND)
	set(HTTPS_SUPPORT_DEFAULT OFF)
endif (NOT OPENSSL_FOUND)

option(SUPPORT_HTTPS "whether to support https" ${SUPPORT_HTTPS_DEFAULT})

include_directories(${LIBEV_INCLUDE_DIRS} ${HTTPPARSER_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

set(EXTRA_LIBS ${LIBEV_LIBRARIES} ${HTTPPARSER_LIBRARIES})
set(SOURCE_FILES emn.c emn_str.c emn_buf.c emn_utils.c emn_http.c)

if (SUPPORT_HTTPS)
	set(EMN_SUPPORT_HTTPS 1)
	include_directories(${OPENSSL_INCLUDE_DIRS})
	list(APPEND EXTRA_LIBS ${OPENSSL_LIBRARIES})
	list(APPEND SOURCE_FILES emn_ssl.c)
endif (SUPPORT_HTTPS)

add_library(emn SHARED ${SOURCE_FILES})
set_target_properties(emn PROPERTIES VERSION ${EMN_VERSION_MAJOR}.${EMN_VERSION_MINOR})
target_link_libraries(emn ${EXTRA_LIBS})

# configure a header file to pass some of the CMake settings to the source code
configure_file(emn_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/emn_config.h)

install(
	FILES emn.h ${CMAKE_CURRENT_BINARY_DIR}/emn_config.h
	DESTINATION include
)

install(
	TARGETS emn LIBRARY
	DESTINATION lib
)
