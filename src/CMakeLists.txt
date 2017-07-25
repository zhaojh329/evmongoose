# The version number.
set(EMN_VERSION_MAJOR 0)
set(EMN_VERSION_MINOR 1)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Check the necessary Libraries

find_package(Libev REQUIRED)
find_package(HttpParser REQUIRED)

include_directories(${LIBEV_INCLUDE_DIRS} ${HTTPPARSER_INCLUDE_DIRS})
set(EXTRA_LIBS ${LIBEV_LIBRARIES} ${HTTPPARSER_LIBRARIES})
set(SOURCE_FILES emn.c emn_str.c emn_buf.c emn_utils.c emn_http.c)

# Check SSL libraries. TODO support OpenSSL, CyaSSL

set(SSL_ENABLED 0)
option(CMAKE_USE_CYASSL "Enable CyaSSL for SSL/TLS" OFF)
set(openssl_default ON)

if(CMAKE_USE_CYASSL)
	set(openssl_default OFF)
endif()

option(CMAKE_USE_OPENSSL "Enable OpenSSL for SSL/TLS" ${openssl_default})

foreach(option_var IN LISTS CMAKE_USE_CYASSL CMAKE_USE_OPENSSL)
	if(${option_var})
		list(APPEND enabled_ssl_options ${option_var})
	endif()
endforeach()
list(LENGTH enabled_ssl_options enabled_ssl_options_count)

if(enabled_ssl_options_count GREATER 1)
	message(FATAL_ERROR "Multiple SSL options specified. Please pick at most one and disable the rest.")
endif()

set(USE_OPENSSL 0)

if(CMAKE_USE_OPENSSL)
	find_package(OpenSSL REQUIRED)
	set(SSL_ENABLED 1)
	set(USE_OPENSSL 1)
	list(APPEND EXTRA_LIBS ${OPENSSL_LIBRARIES})
	include_directories(${OPENSSL_INCLUDE_DIR})
endif()

set(USE_CYASSL 0)

if(CMAKE_USE_CYASSL)
	find_package(CyaSSL REQUIRED)
	set(SSL_ENABLED 1)
	set(USE_CYASSL 1)
	list(APPEND EXTRA_LIBS ${CYASSL_LIBRARIES})
	include_directories(${CYASSL_INCLUDE_DIR})
endif()

if(SSL_ENABLED)
	list(APPEND SOURCE_FILES emn_ssl.c)
endif()

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
