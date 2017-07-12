find_library(LIBEV_LIBRARY NAMES ev)
find_path(LIBEV_INCLUDE_DIR ev.h PATH_SUFFIXES include)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libev DEFAULT_MSG LIBEV_LIBRARY LIBEV_INCLUDE_DIR)
if (NOT LIBEV_FOUND)
	message(FATAL_ERROR "libev not found")
endif (NOT LIBEV_FOUND)
 
find_library(LIBMBEDTLS mbedtls)
find_package(OpenSSL)

set(HTTPS_SUPPORT_DEFAULT "ON")

if (NOT OPENSSL_FOUND AND NOT LIBMBEDTLS)
	set(HTTPS_SUPPORT_DEFAULT "OFF")
endif (NOT OPENSSL_FOUND AND NOT LIBMBEDTLS)

option(HTTPS_SUPPORT "whether to support https" ${HTTPS_SUPPORT_DEFAULT})
option(USE_MBEDTLS "whether force use mbedtls to support https" OFF)

add_library(evmongoose SHARED mongoose.c)
target_link_libraries(evmongoose ${LIBEV_LIBRARY})

if (HTTPS_SUPPORT)
	if (NOT OPENSSL_FOUND AND NOT LIBMBEDTLS)
		MESSAGE(FATAL_ERROR "OpenSSL or mbedtls not found")
	endif (NOT OPENSSL_FOUND AND NOT LIBMBEDTLS)
	
	if (USE_MBEDTLS AND NOT LIBMBEDTLS)
		MESSAGE(FATAL_ERROR "mbedtls not found")
	endif (USE_MBEDTLS AND NOT LIBMBEDTLS)
	
	add_definitions(-DMG_ENABLE_SSL)
	
	if (USE_MBEDTLS)
		add_definitions(-DMG_SSL_IF=MG_SSL_IF_MBEDTLS -DMG_SSL_MBED_DUMMY_RANDOM)
		target_link_libraries(evmongoose mbedcrypto mbedtls mbedx509)
	else ()
		target_link_libraries(evmongoose ${OPENSSL_LIBRARIES})
	endif (USE_MBEDTLS)
endif (HTTPS_SUPPORT)

add_subdirectory(lua)

install(FILES mongoose.h
	DESTINATION include
)

install(TARGETS evmongoose
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
)
