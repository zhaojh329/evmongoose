cmake_minimum_required(VERSION 3.0)

project(emn C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules/")

#set(CMAKE_VERBOSE_MAKEFILE ON)

# The version number.
set(EMN_VERSION_MAJOR 0)
set(EMN_VERSION_MINOR 1)

find_package(Libev 4.22 REQUIRED)
find_package(HttpParser 2.1 REQUIRED)

add_definitions(-O -Wall -Werror --std=gnu99 -g3)

include_directories(${CMAKE_SOURCE_DIR}/src)

add_subdirectory(src)
add_subdirectory(example)