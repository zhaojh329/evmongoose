find_library(LIBEV_LIBRARY NAMES ev)
find_path(LIBEV_INCLUDE_DIR ev.h PATH_SUFFIXES include)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libev DEFAULT_MSG LIBEV_LIBRARY LIBEV_INCLUDE_DIR)
if (NOT LIBEV_FOUND)
	message(FATAL_ERROR "libev not found")
endif (NOT LIBEV_FOUND)

add_library(emn SHARED emn.c)
target_link_libraries(emn ${LIBEV_LIBRARY})

install(FILES emn.h
	DESTINATION include
)

install(TARGETS emn
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
)
