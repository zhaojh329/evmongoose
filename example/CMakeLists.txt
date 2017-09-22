add_executable(simplest_web simplest_web.c)
target_link_libraries(simplest_web emn)

add_executable(async_dns_resolver async_dns_resolver.c)
target_link_libraries(async_dns_resolver emn)

add_executable(tcp_client tcp_client.c)
target_link_libraries(tcp_client emn)

add_executable(http_client http_client.c)
target_link_libraries(http_client emn)