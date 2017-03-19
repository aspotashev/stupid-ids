project(StupidsRuby)
cmake_minimum_required(VERSION 2.6)

unset(RUBY_VERSION_MAJOR CACHE)
unset(RUBY_INCLUDE_DIR CACHE)
unset(RUBY_CONFIG_INCLUDE_DIR CACHE)
unset(RUBY_LIBRARY CACHE)
find_package(Ruby 1.9 REQUIRED)

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

install(TARGETS stupidsruby
    LIBRARY DESTINATION ${RUBY_SITEARCH_DIR}
)
