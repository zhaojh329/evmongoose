cmake_minimum_required (VERSION 2.6)

project(evmongoose C)

#set(CMAKE_VERBOSE_MAKEFILE ON)

add_definitions(-O -Wall -Werror --std=gnu99)

include_directories(${CMAKE_SOURCE_DIR})

find_library(LIBEV ev)
find_library(LIBMBEDTLS mbedtls)
find_library(LIBMBEDCRYPTO mbedcrypto)
find_library(LIBMBEDX509 mbedx509)
find_package(OpenSSL)

if (NOT LIBEV)
	message(FATAL_ERROR "libev not found")
endif (NOT LIBEV)

set(HTTPS_SUPPORT_DEFAULT "ON")

if (NOT OPENSSL_FOUND AND (NOT LIBMBEDTLS OR NOT LIBMBEDCRYPTO OR NOT LIBMBEDX509))
	set(HTTPS_SUPPORT_DEFAULT "OFF")
endif (NOT OPENSSL_FOUND AND (NOT LIBMBEDTLS OR NOT LIBMBEDCRYPTO OR NOT LIBMBEDX509))

option(HTTPS_SUPPORT "whether to support https" ${HTTPS_SUPPORT_DEFAULT})
option(USE_MBEDTLS "whether force use mbedtls to support https" OFF)

add_library(evmongoose SHARED mongoose.c)
target_link_libraries(evmongoose ${LIBEV})

if (HTTPS_SUPPORT)
	if (NOT OPENSSL_FOUND AND (NOT LIBMBEDTLS OR NOT LIBMBEDCRYPTO OR NOT LIBMBEDX509))
		MESSAGE(FATAL_ERROR "OpenSSL or mbedtls not found")
	endif (NOT OPENSSL_FOUND AND (NOT LIBMBEDTLS OR NOT LIBMBEDCRYPTO OR NOT LIBMBEDX509))
	
	if (USE_MBEDTLS AND (NOT LIBMBEDTLS OR NOT LIBMBEDCRYPTO OR NOT LIBMBEDX509))
		MESSAGE(FATAL_ERROR "mbedtls not found")
	endif (USE_MBEDTLS AND (NOT LIBMBEDTLS OR NOT LIBMBEDCRYPTO OR NOT LIBMBEDX509))
	
	add_definitions(-DMG_ENABLE_SSL)

	if (USE_MBEDTLS)
		add_definitions(-DMG_SSL_IF=MG_SSL_IF_MBEDTLS -DMG_SSL_MBED_DUMMY_RANDOM)
		target_link_libraries(evmongoose mbedcrypto mbedtls mbedx509)
	else ()
		target_link_libraries(evmongoose ${OPENSSL_LIBRARIES})
	endif (USE_MBEDTLS)
endif (HTTPS_SUPPORT)

add_subdirectory(example) 

install(FILES mongoose.h
	DESTINATION include
)

install(TARGETS evmongoose
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
)
