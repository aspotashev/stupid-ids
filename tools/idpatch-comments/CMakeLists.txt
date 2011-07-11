project (idpatch-comments)
cmake_minimum_required (VERSION 2.8.0)

set (idpatch_comments_SRCS
	main.cpp
)

add_executable (idpatch-comments ${idpatch_comments_SRCS})
target_link_libraries (idpatch-comments gettextpo_helper)

set_target_properties (idpatch-comments PROPERTIES COMPILE_FLAGS "-ggdb")

install (TARGETS idpatch-comments DESTINATION bin)
