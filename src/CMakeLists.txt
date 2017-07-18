include_directories(${LIBEV_INCLUDE_DIRS} ${HTTP_PARSER_INCLUDE_DIRS})

add_library(emn SHARED emn.c ebuf.c)
set_target_properties(emn PROPERTIES VERSION 0.1)
target_link_libraries(emn ${LIBEV_LIBRARIES} ${HTTP_PARSER_LIBRARIES})

install(FILES emn.h
	DESTINATION include
)

install(TARGETS emn
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
)
