project(lokalize-review-iddiff)
cmake_minimum_required(VERSION 2.8.0)

set(lokalize_review_iddiff_SRCS
    main.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/gettextpo-helper)

add_executable(lokalize-review-iddiff ${lokalize_review_iddiff_SRCS})
target_link_libraries(lokalize-review-iddiff gettextpo_helper dbuslokalize)

set_target_properties(lokalize-review-iddiff PROPERTIES COMPILE_FLAGS "-ggdb")

install(TARGETS lokalize-review-iddiff DESTINATION bin)
