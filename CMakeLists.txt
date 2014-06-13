cmake_minimum_required(VERSION 2.8)

set(GETTEXTPO_HELPER_INCLUDES
	${CMAKE_SOURCE_DIR}/gettextpo-helper/lib/include
	${CMAKE_SOURCE_DIR}/dbuslokalize
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

include_directories(${GETTEXTPO_HELPER_INCLUDES})

enable_testing()

add_subdirectory(globalize)
add_subdirectory(gettextpo-helper)
add_subdirectory(dbuslokalize)
add_subdirectory(tools)
add_subdirectory(stupids-server)
add_subdirectory(examples)
add_subdirectory(tests)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

# uninstall target
configure_file(
    "${CMAKE_MODULE_PATH}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
