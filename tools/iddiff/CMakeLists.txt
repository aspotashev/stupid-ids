project(iddiff)
cmake_minimum_required(VERSION 2.8.0)

set(iddiff_SRCS
    main.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/gettextpo-helper)

add_executable(iddiff ${iddiff_SRCS})
target_link_libraries(iddiff gettextpo_helper)

set_target_properties(iddiff PROPERTIES COMPILE_FLAGS "-ggdb")

install(TARGETS iddiff DESTINATION bin)
