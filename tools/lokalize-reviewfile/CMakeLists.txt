project (lokalize-reviewfile)
cmake_minimum_required (VERSION 2.8.0)

set (lokalize_reviewfile_SRCS
	main.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/gettextpo-helper)

add_executable (lokalize-reviewfile ${lokalize_reviewfile_SRCS})
target_link_libraries (lokalize-reviewfile gettextpo_helper dbuslokalize)

set_target_properties (lokalize-reviewfile PROPERTIES COMPILE_FLAGS "-ggdb")

install (TARGETS lokalize-reviewfile DESTINATION bin)
