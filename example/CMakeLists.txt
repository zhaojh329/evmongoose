add_executable(simplest_web simplest_web.c)
target_link_libraries(simplest_web evmongoose)

if (HTTPS_SUPPORT)
	add_executable(simplest_web_ssl simplest_web_ssl.c)
	target_link_libraries(simplest_web_ssl evmongoose)
endif (HTTPS_SUPPORT)

add_executable(http_client http_client.c)
target_link_libraries(http_client evmongoose)

add_executable(async_dns_resolver async_dns_resolver.c)
target_link_libraries(async_dns_resolver evmongoose)

add_executable(tcp_echo_server tcp_echo_server.c)
target_link_libraries(tcp_echo_server evmongoose)

