project(globalize)
cmake_minimum_required (VERSION 2.6.0)
find_package (Qt4 REQUIRED)
include (UseQt4)

#include_directories (${QT_QTGUI_INCLUDE_DIR} ${QT_QTCORE_INCLUDE_DIR})
include_directories(
	${QT_INCLUDES}
	${CMAKE_CURRENT_BINARY_DIR}
	${GETTEXTPO_HELPER_INCLUDES}
)

set(globalize_SRCS
	messageheaderwidget.cpp
	messageeditorwidget.cpp
	messagetranslationoption.cpp
	mainwindow.cpp
	main.cpp
	lib/qtranslationcollector.cpp
)

qt4_wrap_ui(globalize_UIS_H
	mainwindow.ui
)

qt4_wrap_cpp(globalize_MOCS
	mainwindow.h
	messageeditorwidget.h
	messagetranslationoption.h
)

add_executable(globalize ${globalize_SRCS} ${globalize_UIS_H} ${globalize_MOCS})
target_link_libraries (globalize ${QT_LIBRARIES})
target_link_libraries (globalize gettextpo_helper)

set_target_properties (globalize PROPERTIES COMPILE_FLAGS "-ggdb")


add_subdirectory(conflicting-translations)

