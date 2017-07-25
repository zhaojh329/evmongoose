cmake_minimum_required (VERSION 2.6)

project(evmongoose C)

#set(CMAKE_VERBOSE_MAKEFILE ON)

add_definitions(-O -Wall -Werror --std=gnu99 -g3)
add_definitions(-DMG_ENABLE_HTTP_STREAMING_MULTIPART)

option(ENABLE_MQTT_BROKER "whether support mqtt broker" ON)

if (ENABLE_MQTT_BROKER)
	add_definitions(-DMG_ENABLE_MQTT_BROKER)
endif (ENABLE_MQTT_BROKER)

include_directories(${CMAKE_SOURCE_DIR}/src)

add_subdirectory(src)
add_subdirectory(example)
