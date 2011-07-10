project (stupids-rerere-trusted-filter)
cmake_minimum_required (VERSION 2.8.0)

set (stupids_rerere_trusted_filter_SRCS
	main.cpp
)

add_executable (stupids-rerere-trusted-filter ${stupids_rerere_trusted_filter_SRCS})
target_link_libraries (stupids-rerere-trusted-filter gettextpo_helper)

set_target_properties (stupids-rerere-trusted-filter PROPERTIES COMPILE_FLAGS "-ggdb")

install (TARGETS stupids-rerere-trusted-filter DESTINATION bin)
