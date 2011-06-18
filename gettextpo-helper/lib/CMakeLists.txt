cmake_minimum_required(VERSION 2.8)
project(gettextpo_helper)

set(gettextpo_helper_LIB_SRCS
	gettextpo-helper.cpp
	translation-collector.cpp
	mappedfile.cpp
	translationcontent.cpp
	stupids-client.cpp
	oidmapcache.cpp
	iddiff.cpp
	message.cpp

	detector.cpp
	detectorbase.cpp
	gitloader.cpp
	processorphans.cpp
)

add_library(gettextpo_helper SHARED ${gettextpo_helper_LIB_SRCS})
target_link_libraries(gettextpo_helper
	${QT_QTCORE_LIBRARY} # for QCryptographicHash
	cryptopp
	gettextpo
	git2
)

SET(INCLUDE_INSTALL_DIR include) # what's the full path?
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
	DESTINATION ${INCLUDE_INSTALL_DIR}/gettextpo-helper COMPONENT Devel
)

set(INSTALL_TARGETS_DEFAULT_ARGS
	LIBRARY DESTINATION lib)
install(TARGETS gettextpo_helper ${INSTALL_TARGETS_DEFAULT_ARGS})

set_target_properties (gettextpo_helper PROPERTIES COMPILE_FLAGS "-ggdb")

