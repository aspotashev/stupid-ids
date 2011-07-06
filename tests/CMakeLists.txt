set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${KOMAIN_INCLUDES} )

add_definitions(-DFILES_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/")

########### next target ###############

add_executable(test-sha1-buffer sha1-buffer.cpp)
target_link_libraries(test-sha1-buffer gettextpo_helper)
add_test(test-sha1-buffer ${CMAKE_CURRENT_BINARY_DIR}/test-sha1-buffer)

########### next target ###############

add_executable(list_equal_messages_ids list_equal_messages_ids.cpp)
target_link_libraries(list_equal_messages_ids gettextpo_helper)
add_test(list_equal_messages_ids ${CMAKE_CURRENT_BINARY_DIR}/list_equal_messages_ids)
add_test(list_equal_messages_ids_diff diff -q ${CMAKE_CURRENT_SOURCE_DIR}/ref/list_equal_messages_ids.txt ${CMAKE_CURRENT_BINARY_DIR}/result.txt)

########### next target ###############

#set(findmatch_test_SRCS testfindmatch.cpp )
#kde4_add_unit_test(testfindmatch TESTNAME libs-komain-testfindmatch ${findmatch_test_SRCS})
#target_link_libraries(testfindmatch komain ${QT_QTTEST_LIBRARY})

