cmake_minimum_required(VERSION 2.8)
project(dbuslokalize)


# DBus
find_package(PkgConfig REQUIRED)
pkg_check_modules(dbus-glib REQUIRED dbus-glib-1)
include_directories(${dbus-glib_INCLUDE_DIRS})

set(dbuslokalize_LIB_SRCS
	dbuslokalize.cpp
)

add_library(dbuslokalize SHARED ${dbuslokalize_LIB_SRCS})
#target_link_libraries(dbuslokalize
#	gettextpo
#)

SET(INCLUDE_INSTALL_DIR include) # what's the full path?
install(FILES
	dbuslokalize.h
	DESTINATION ${INCLUDE_INSTALL_DIR} COMPONENT Devel
)

set(INSTALL_TARGETS_DEFAULT_ARGS
	LIBRARY DESTINATION lib)
install(TARGETS dbuslokalize ${INSTALL_TARGETS_DEFAULT_ARGS})

set_target_properties (dbuslokalize PROPERTIES COMPILE_FLAGS "-ggdb")

