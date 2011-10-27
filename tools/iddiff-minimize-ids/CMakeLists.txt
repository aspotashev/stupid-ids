project (iddiff-minimize-ids)
cmake_minimum_required (VERSION 2.8.0)

set (iddiff_minimize_ids_SRCS
	main.cpp
)

add_executable (iddiff-minimize-ids ${iddiff_minimize_ids_SRCS})
target_link_libraries (iddiff-minimize-ids gettextpo_helper)

set_target_properties (iddiff-minimize-ids PROPERTIES COMPILE_FLAGS "-ggdb")

install (TARGETS iddiff-minimize-ids DESTINATION bin)
