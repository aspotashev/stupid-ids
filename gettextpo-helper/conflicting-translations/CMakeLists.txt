project(conflicting-translations)

cmake_minimum_required (VERSION 2.8.0)
find_package (Qt4 REQUIRED)

include (UseQt4)

include_directories (${QT_QTGUI_INCLUDE_DIR} ${QT_QTCORE_INCLUDE_DIR})

set(ct_SRCS
	main.cpp
	qtranslationcollector.cpp
)

add_executable (conflicting-translations ${ct_SRCS})
target_link_libraries (conflicting-translations ${QT_LIBRARIES})
target_link_libraries (conflicting-translations gettextpo_helper)

set_target_properties (conflicting-translations PROPERTIES COMPILE_FLAGS "-ggdb")

