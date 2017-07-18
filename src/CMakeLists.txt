include_directories(${LIBEV_INCLUDE_DIR})

add_library(emn SHARED emn.c)
target_link_libraries(emn ${LIBEV_LIBRARY})

install(FILES emn.h
	DESTINATION include
)

install(TARGETS emn
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
)
