cmake_minimum_required (VERSION 2.6)

project(evmongoose C)

add_definitions(-Wall -Werror --std=gnu99)

include_directories(${CMAKE_SOURCE_DIR})

add_library(evmongoose SHARED mongoose.c)
target_link_libraries(evmongoose ev)

add_executable(example example.c)
target_link_libraries(example evmongoose)

install(TARGETS evmongoose
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
)
