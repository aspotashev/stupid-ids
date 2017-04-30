
set(INPUT_DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/data)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

include_directories(${CMAKE_SOURCE_DIR}/gettextpo-helper)

add_library(common OBJECT common.cpp)

macro(add_unit_test testname)
    add_executable(test_${testname}
        test_${testname}.cpp
        $<TARGET_OBJECTS:common>
    )
    target_link_libraries(test_${testname}
        gettextpo_helper

        ${GTEST_BOTH_LIBRARIES}
        pthread
    )
    set_target_properties(test_${testname}
        PROPERTIES
        COMPILE_FLAGS
        -DINPUT_DATA_DIR=\\"${INPUT_DATA_DIR}\\")
    add_test(test_${testname} test_${testname})
endmacro(add_unit_test testname)

enable_testing()
add_unit_test(calculatetphash)
add_unit_test(sha1buffer)
add_unit_test(iddiff)
add_unit_test(matchpomessages)
