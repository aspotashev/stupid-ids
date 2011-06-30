cmake_minimum_required(VERSION 2.8)

set(GETTEXTPO_HELPER_INCLUDES
	${CMAKE_SOURCE_DIR}/gettextpo-helper/lib/include
	${CMAKE_SOURCE_DIR}/dbuslokalize
)

include_directories(${GETTEXTPO_HELPER_INCLUDES})

add_subdirectory(globalize)
add_subdirectory(gettextpo-helper)
add_subdirectory(dbuslokalize)
add_subdirectory(tools)
add_subdirectory(stupids-server)
add_subdirectory(examples)

