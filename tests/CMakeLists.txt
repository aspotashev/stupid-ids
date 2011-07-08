set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${KOMAIN_INCLUDES} )

macro(add_unit_test testname)
	add_executable(${testname} ${testname}/${testname}.cpp)
	target_link_libraries(${testname} gettextpo_helper)
	set_target_properties(${testname}
		PROPERTIES
		COMPILE_FLAGS
		-DINPUT_DATA_DIR=\\"${CMAKE_CURRENT_SOURCE_DIR}/${testname}/data/\\")
	add_test(${testname} ${CMAKE_CURRENT_BINARY_DIR}/${testname})
endmacro(add_unit_test testname)

########### next target ###############

add_unit_test(sha1_buffer)

########### next target ###############

add_unit_test(list_equal_messages_ids)
add_test(list_equal_messages_ids_diff diff -q ${CMAKE_CURRENT_SOURCE_DIR}/ref/list_equal_messages_ids.txt ${CMAKE_CURRENT_BINARY_DIR}/result.txt)

