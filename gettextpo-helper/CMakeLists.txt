cmake_minimum_required(VERSION 2.8)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_definitions(-std=c++14)

add_subdirectory(gtpo)
add_subdirectory(ruby-ext)
