project (app)
cmake_minimum_required (VERSION 2.8.0)

set (app_SRCS
	main.cpp
)

add_executable (app ${app_SRCS})
target_link_libraries (app gettextpo_helper)
set_target_properties (app PROPERTIES COMPILE_FLAGS "-ggdb")

