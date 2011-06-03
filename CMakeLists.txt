cmake_minimum_required(VERSION 2.8)

set(GETTEXTPO_HELPER_INCLUDES
	${CMAKE_SOURCE_DIR}/gettextpo-helper/lib/include
)

add_subdirectory(globalize)
add_subdirectory(gettextpo-helper)

