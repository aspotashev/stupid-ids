project (iddiff-merge)
cmake_minimum_required (VERSION 2.8.0)

set (iddiff_merge_SRCS
	main.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/gettextpo-helper)

add_executable (iddiff-merge ${iddiff_merge_SRCS})
target_link_libraries (iddiff-merge gettextpo_helper)

set_target_properties (iddiff-merge PROPERTIES COMPILE_FLAGS "-ggdb")

install (TARGETS iddiff-merge DESTINATION bin)
