cmake_minimum_required (VERSION 2.6)

project(emn C)

#set(CMAKE_VERBOSE_MAKEFILE ON)

add_definitions(-O -Wall -Werror --std=gnu99 -g3)

include_directories(${CMAKE_SOURCE_DIR}/src)

add_subdirectory(src)
add_subdirectory(example)