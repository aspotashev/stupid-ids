project (iddiff-merge-trcomm)
cmake_minimum_required (VERSION 2.8.0)

set (iddiff_merge_trcomm_SRCS
	main.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/gettextpo-helper)

add_executable (iddiff-merge-trcomm ${iddiff_merge_trcomm_SRCS})
target_link_libraries (iddiff-merge-trcomm gettextpo_helper)

set_target_properties (iddiff-merge-trcomm PROPERTIES COMPILE_FLAGS "-ggdb")

install (TARGETS iddiff-merge-trcomm DESTINATION bin)
