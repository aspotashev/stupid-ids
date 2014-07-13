project (gtpo-multitool)
cmake_minimum_required (VERSION 2.8.0)

set (gtpo_multitool_SRCS
    calculate-tp-hash/main.cpp
    iddiff/main.cpp
    iddiff-git/main.cpp
    iddiff-merge/main.cpp
    iddiff-merge-trcomm/main.cpp
    iddiff-minimize-ids/main.cpp
    iddiff-repo/main.cpp
    iddiff-review-format-mailbox/main.cpp
    idpatch/main.cpp
    lokalize-reviewfile/main.cpp
    lokalize-review-iddiff/main.cpp
    stupids-rerere-trusted-filter/main.cpp
    stupids-reverse-tphash/main.cpp

    main.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/gettextpo-helper)

add_executable(gtpo-multitool ${gtpo_multitool_SRCS})
target_link_libraries(gtpo-multitool gettextpo_helper dbuslokalize)

set_target_properties(gtpo-multitool PROPERTIES COMPILE_FLAGS "-ggdb")

install(TARGETS gtpo-multitool DESTINATION bin)


add_subdirectory(iddiff-test)
