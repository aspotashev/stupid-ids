
find_package(Boost COMPONENTS unit_test_framework REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/gettextpo-helper)

macro(add_unit_test testname)
    set(INPUT_DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/data)

    add_executable(test_${testname} test_${testname}.cpp)
    target_link_libraries(test_${testname} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
    target_link_libraries(test_${testname} gettextpo_helper)
    set_target_properties(test_${testname}
        PROPERTIES
        COMPILE_FLAGS
        -DINPUT_DATA_DIR=\\"${INPUT_DATA_DIR}\\")
    add_test(${testname} ${CMAKE_CURRENT_BINARY_DIR}/test_${testname})
endmacro(add_unit_test testname)

enable_testing()
add_unit_test(calculatetphash)
add_unit_test(sha1buffer)
add_unit_test(iddiff)
