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

# 词法分析测试（后续模块）
# add_executable(test_lexer test_lexer.cpp)
# target_link_libraries(test_lexer PRIVATE parser test_utils)
# add_test(NAME lexer_tests COMMAND test_lexer)
