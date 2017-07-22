# The version number.
set(EMN_VERSION_MAJOR 0)
set(EMN_VERSION_MINOR 1)

# configure a header file to pass some of the CMake settings to the source code
configure_file(emn_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/emn_config.h)
	
include_directories(${LIBEV_INCLUDE_DIRS} ${HTTPPARSER_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

add_library(emn SHARED emn.c emn_buf.c emn_utils.c emn_http.c)
set_target_properties(emn PROPERTIES VERSION ${EMN_VERSION_MAJOR}.${EMN_VERSION_MINOR})
target_link_libraries(emn ${LIBEV_LIBRARIES} ${HTTPPARSER_LIBRARIES})

install(
	FILES emn.h ${CMAKE_CURRENT_BINARY_DIR}/emn_config.h
	DESTINATION include
)
install(
	TARGETS emn LIBRARY
	DESTINATION lib
)
