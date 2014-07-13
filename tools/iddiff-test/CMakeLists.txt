project(iddiff-test)
cmake_minimum_required(VERSION 2.8.0)

set(iddiff_test_SRCS
    main.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/gettextpo-helper)

add_executable(iddiff-test ${iddiff_test_SRCS})
target_link_libraries(iddiff-test gettextpo_helper)

set_target_properties(iddiff-test PROPERTIES COMPILE_FLAGS "-ggdb")

install(TARGETS iddiff-test DESTINATION bin)
