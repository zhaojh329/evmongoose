add_executable(simplest_web simplest_web.c)
target_link_libraries(simplest_web evmongoose)

if (HTTPS_SUPPORT)
	add_definitions(-DMG_ENABLE_SSL)
	
	add_executable(simplest_web_ssl simplest_web_ssl.c)
	target_link_libraries(simplest_web_ssl evmongoose)
endif (HTTPS_SUPPORT)

add_executable(http_client http_client.c)
target_link_libraries(http_client evmongoose)

add_executable(async_dns_resolver async_dns_resolver.c)
target_link_libraries(async_dns_resolver evmongoose)

add_executable(tcp_echo_server tcp_echo_server.c)
target_link_libraries(tcp_echo_server evmongoose)

add_executable(tcp_client tcp_client.c)
target_link_libraries(tcp_client evmongoose)

add_executable(mqtt_client mqtt_client.c)
target_link_libraries(mqtt_client evmongoose)

if (ENABLE_MQTT_BROKER)
	add_executable(mqtt_broker mqtt_broker.c)
	target_link_libraries(mqtt_broker evmongoose)
endif (ENABLE_MQTT_BROKER)
	
add_executable(websocket_server websocket_server.c)
target_link_libraries(websocket_server evmongoose)

add_executable(big_upload big_upload.c)
target_link_libraries(big_upload evmongoose)