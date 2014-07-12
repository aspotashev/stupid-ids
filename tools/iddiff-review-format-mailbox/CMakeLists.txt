project(iddiff-review-format-mailbox)
cmake_minimum_required(VERSION 2.8.0)

set(iddiff_review_format_mailbox_SRCS
    main.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/gettextpo-helper)

add_executable(iddiff-review-format-mailbox ${iddiff_review_format_mailbox_SRCS})
target_link_libraries(iddiff-review-format-mailbox gettextpo_helper)

set_target_properties(iddiff-review-format-mailbox PROPERTIES COMPILE_FLAGS "-ggdb")

install(TARGETS iddiff-review-format-mailbox DESTINATION bin)
