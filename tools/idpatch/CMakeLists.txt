project(idpatch)
cmake_minimum_required(VERSION 2.8.0)

set(idpatch_SRCS
    main.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/gettextpo-helper)

add_executable(idpatch ${idpatch_SRCS})
target_link_libraries(idpatch gettextpo_helper)

set_target_properties(idpatch PROPERTIES COMPILE_FLAGS "-ggdb")

install(TARGETS idpatch DESTINATION bin)
