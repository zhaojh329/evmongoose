include_directories(${LIBEV_INCLUDE_DIRS} ${HTTP_PARSER_INCLUDE_DIRS})

add_library(emn SHARED emn.c ebuf.c)
set_target_properties(emn PROPERTIES VERSION ${EMN_VERSION_MAJOR}.${EMN_VERSION_MINOR})
target_link_libraries(emn ${LIBEV_LIBRARIES} ${HTTP_PARSER_LIBRARIES})

install(FILES emn.h DESTINATION include/emn)
install(TARGETS emn	LIBRARY DESTINATION lib)
