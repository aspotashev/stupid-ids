cmake_minimum_required(VERSION 2.8)
project(gettextpo_helper)

set(gettextpo_helper_LIB_SRCS
    gitoid.cpp
    optstring.cpp

    commitfilechange.cpp
    commit.cpp
    repository.cpp

    iddiffmessage.cpp
    iddiffchange.cpp
    iddiff.cpp

    messageoriginaltext.cpp
    messagetranslationbase.cpp
    message.cpp
    messagegroup.cpp

    filecontentbase.cpp
    filecontentfs.cpp
    filecontentgit.cpp
    filecontentbuffer.cpp

    gettextpo-helper.cpp
    translation-collector.cpp
    mappedfile.cpp
    translationcontent.cpp
    stupids-client.cpp
    oidmapcache.cpp
    filedatetime.cpp
    config.cpp
    filedbfirstids.cpp

    detector.cpp
    detectorbase.cpp
    gitloader.cpp
    processorphans.cpp

    block-sha1/sha1.cpp

    exceptionmessageinfo.cpp
    gettextparserexception.cpp
)

add_library(gettextpo_helper SHARED ${gettextpo_helper_LIB_SRCS})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
find_package(Libgit2 REQUIRED 0.22)
include_directories(${LIBGIT2_INCLUDE_DIR})
target_link_libraries(gettextpo_helper
    gettextpo
    ${LIBGIT2_LIBRARIES}
)

set(INCLUDE_INSTALL_DIR include) # what's the full path?
install(FILES
    gettextpo-helper.h
    translation-collector.h
    mappedfile.h
    translationcontent.h
    stupids-client.h
    detectorbase.h
    detector.h
    iddiff.h
    gitloader.h
    message.h
    filedatetime.h
    filedbfirstids.h
    oidmapcache.h
    DESTINATION ${INCLUDE_INSTALL_DIR}/gettextpo-helper COMPONENT Devel
)

set(INSTALL_TARGETS_DEFAULT_ARGS
    LIBRARY DESTINATION lib
)

install(TARGETS gettextpo_helper ${INSTALL_TARGETS_DEFAULT_ARGS})

set_target_properties(gettextpo_helper PROPERTIES COMPILE_FLAGS "-ggdb")
