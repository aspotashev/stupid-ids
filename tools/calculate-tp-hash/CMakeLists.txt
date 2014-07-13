project(tphash)
cmake_minimum_required(VERSION 2.8.0)

set(tphash_SRCS
    main.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/gettextpo-helper)

add_executable(calculate-tp-hash ${tphash_SRCS})
target_link_libraries(calculate-tp-hash gettextpo_helper)

set_target_properties(calculate-tp-hash PROPERTIES COMPILE_FLAGS "-ggdb")

install(TARGETS calculate-tp-hash DESTINATION bin)
