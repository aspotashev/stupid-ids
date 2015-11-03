set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_SOURCE_DIR}/gettextpo-helper)

macro(add_unit_test testname)
	set(INPUT_DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${testname}/data/)
	set(REF_DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${testname}/ref/)

	add_executable(${testname} ${testname}/${testname}.cpp)
	target_link_libraries(${testname} gettextpo_helper)
	set_target_properties(${testname}
		PROPERTIES
		COMPILE_FLAGS
		-DINPUT_DATA_DIR=\\"${INPUT_DATA_DIR}\\")
	add_test(${testname} ${CMAKE_CURRENT_BINARY_DIR}/${testname})
endmacro(add_unit_test testname)

########### next target ###############

add_unit_test(list_equal_messages_ids)
add_test(list_equal_messages_ids_diff diff -q ${REF_DATA_DIR}/list_equal_messages_ids.txt ${CMAKE_CURRENT_BINARY_DIR}/result.txt)

########### next target ###############

add_unit_test(iddiff_merge_headers)
add_test(iddiff_merge_headers_diff diff -qw ${REF_DATA_DIR}/merged.iddiff ${CMAKE_CURRENT_BINARY_DIR}/result.iddiff)

