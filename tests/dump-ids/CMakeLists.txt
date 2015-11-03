include_directories(${CMAKE_SOURCE_DIR}/gettextpo-helper)

add_executable(dump_ids dump-ids.cpp)
target_link_libraries(dump_ids gettextpo_helper)
