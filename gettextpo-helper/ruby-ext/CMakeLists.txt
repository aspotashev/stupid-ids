project(StupidsRuby)
cmake_minimum_required(VERSION 2.6)

find_package(Ruby REQUIRED)

include_directories(${RUBY_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/lib/include)
set(stupidsruby_LIB_SRCS
	stupidsruby.cpp

	module.cpp

	filedbfirstids.cpp
	gitloader.cpp
	globals.cpp
	idmapdb.cpp
	message.cpp
	translationcontent.cpp
	)

add_library(stupidsruby SHARED ${stupidsruby_LIB_SRCS})

target_link_libraries(stupidsruby ${RUBY_LIBRARY} gettextpo_helper)
set_target_properties(stupidsruby PROPERTIES PREFIX "")

install(TARGETS stupidsruby DESTINATION ${RUBY_SITEARCH_DIR})

