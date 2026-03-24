add_library(test_utils STATIC
    test_utils.cpp
)

target_include_directories(test_utils PUBLIC
    ${CMAKE_SOURCE_DIR}/src/common
)

# 公共模块测试
add_executable(test_common test_common.cpp)
target_link_libraries(test_common PRIVATE common test_utils)

add_test(NAME common_tests COMMAND test_common)

# 存储模块测试
add_executable(test_storage test_storage.cpp)
target_link_libraries(test_storage PRIVATE storage common test_utils)

add_test(NAME storage_tests COMMAND test_storage)

# 文件生成演示
add_executable(demo_file_creation demo_file_creation.cpp)
target_link_libraries(demo_file_creation PRIVATE storage common)

# 词法分析测试（后续模块）
# add_executable(test_lexer test_lexer.cpp)
# target_link_libraries(test_lexer PRIVATE parser test_utils)
# add_test(NAME lexer_tests COMMAND test_lexer)
