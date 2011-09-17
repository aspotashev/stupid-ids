project (stupids-server)
cmake_minimum_required (VERSION 2.8.0)

set (stupids_server_SRCS
	main.cpp
	tcpcommandserver.cpp
	server.cpp
)

add_executable (stupids-server ${stupids_server_SRCS})
target_link_libraries (stupids-server gettextpo_helper)

set_target_properties (stupids-server PROPERTIES COMPILE_FLAGS "-ggdb")

install (TARGETS stupids-server DESTINATION bin)
