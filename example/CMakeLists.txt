add_executable(simplest_web simplest_web.c)
target_link_libraries(simplest_web evmongoose)

if (HTTPS_SUPPORT)
	add_executable(simplest_web_ssl simplest_web_ssl.c)
	target_link_libraries(simplest_web_ssl evmongoose)
endif (HTTPS_SUPPORT)

add_executable(http_client http_client.c)
target_link_libraries(http_client evmongoose)
