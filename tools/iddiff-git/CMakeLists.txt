project (iddiff-git)
cmake_minimum_required (VERSION 2.8.0)

set (iddiff_git_SRCS
	main.cpp
)

add_executable (iddiff-git ${iddiff_git_SRCS})
target_link_libraries (iddiff-git gettextpo_helper)

set_target_properties (iddiff-git PROPERTIES COMPILE_FLAGS "-ggdb")

install (TARGETS iddiff-git DESTINATION bin)
