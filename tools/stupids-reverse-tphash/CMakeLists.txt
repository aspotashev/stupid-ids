project (stupids-reverse-tphash)
cmake_minimum_required (VERSION 2.8.0)

set (stupids_reverse_tphash_SRCS
	main.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/gettextpo-helper)

add_executable (stupids-reverse-tphash ${stupids_reverse_tphash_SRCS})
target_link_libraries (stupids-reverse-tphash gettextpo_helper)

set_target_properties (stupids-reverse-tphash PROPERTIES COMPILE_FLAGS "-ggdb")

install (TARGETS stupids-reverse-tphash DESTINATION bin)
