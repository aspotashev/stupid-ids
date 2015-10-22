project (gtpo-multitool)
cmake_minimum_required (VERSION 2.8.0)

set (gtpo_multitool_SRCS
    impl/findbestmatchingtranslated.cpp
    impl/iddiff.cpp
    impl/iddiffgit.cpp
    impl/iddiffmerge.cpp
    impl/iddiffmergetrcomm.cpp
    impl/iddiffminimizeids.cpp
    impl/iddiffrepo.cpp
    impl/iddiffreviewformatmailbox.cpp
    impl/idpatch.cpp
    impl/lokalizereviewfile.cpp
    impl/lokalizereviewiddiff.cpp
    impl/rereretrustedfilter.cpp
    impl/reversetphash.cpp

    main.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/gettextpo-helper)

add_executable(gtpo-multitool ${gtpo_multitool_SRCS})
target_link_libraries(gtpo-multitool gettextpo_helper dbuslokalize)

set_target_properties(gtpo-multitool PROPERTIES COMPILE_FLAGS "-ggdb")

install(TARGETS gtpo-multitool DESTINATION bin)


add_subdirectory(iddiff-test)
