cmake_minimum_required (VERSION 2.6)

project(evmongoose C)

#set(CMAKE_VERBOSE_MAKEFILE ON)

add_definitions(-O -Wall -Werror --std=gnu99 -g3)
add_definitions(-DMG_ENABLE_HTTP_STREAMING_MULTIPART)

add_subdirectory(src)
add_subdirectory(example)