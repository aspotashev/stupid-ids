project(iddiff-repo)
cmake_minimum_required(VERSION 2.8.0)

set(iddiff_repo_SRCS
    main.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/gettextpo-helper)

add_executable(iddiff-repo ${iddiff_repo_SRCS})
target_link_libraries(iddiff-repo gettextpo_helper)

set_target_properties(iddiff-repo PROPERTIES COMPILE_FLAGS "-ggdb")

install(TARGETS iddiff-repo DESTINATION bin)
