add_library(test_utils STATIC
    test_utils.cpp
)

target_include_directories(test_utils PUBLIC
    ${CMAKE_SOURCE_DIR}/src/common
)

if(NOT BUILD_SERVER)
    # ============================================
    # GUI 模式测试 (需要 Qt)
    # ============================================
    # 公共模块测试
    add_executable(test_common test_common.cpp)
    target_link_libraries(test_common PRIVATE common test_utils)
    add_test(NAME common_tests COMMAND test_common)

    # 存储模块测试
    add_executable(test_storage test_storage.cpp)
    target_link_libraries(test_storage PRIVATE storage common server_concurrency test_utils)
    add_test(NAME storage_tests COMMAND test_storage)

    # 文件生成演示
    add_executable(demo_file_creation demo_file_creation.cpp)
    target_link_libraries(demo_file_creation PRIVATE storage common)

    # 词法分析测试
    add_executable(test_lexer test_lexer.cpp)
    target_link_libraries(test_lexer PRIVATE parser test_utils)
    add_test(NAME lexer_tests COMMAND test_lexer)

    # AST 测试
    add_executable(test_ast test_ast.cpp)
    target_link_libraries(test_ast PRIVATE parser test_utils)
    add_test(NAME ast_tests COMMAND test_ast)

    # Parser 测试
    add_executable(test_parser test_parser.cpp)
    target_link_libraries(test_parser PRIVATE parser test_utils)
    add_test(NAME parser_tests COMMAND test_parser)

    # 语义分析测试
    add_executable(test_semantic test_semantic.cpp)
    target_link_libraries(test_semantic PRIVATE parser storage test_utils)
    add_test(NAME semantic_tests COMMAND test_semantic)

    # 表达式求值器测试
    add_executable(test_expression_evaluator test_expression_evaluator.cpp)
    target_link_libraries(test_expression_evaluator PRIVATE executor parser common storage test_utils)
    add_test(NAME expression_evaluator_tests COMMAND test_expression_evaluator)

    # 表扫描算子测试
    add_executable(test_table_scan test_table_scan.cpp)
    target_link_libraries(test_table_scan PRIVATE executor parser common storage test_utils)
    add_test(NAME table_scan_tests COMMAND test_table_scan)

    # 过滤算子测试
    add_executable(test_filter_operator test_filter_operator.cpp)
    target_link_libraries(test_filter_operator PRIVATE executor parser common storage test_utils)
    add_test(NAME filter_operator_tests COMMAND test_filter_operator)

    # 投影算子测试
    add_executable(test_project_operator test_project_operator.cpp)
    target_link_libraries(test_project_operator PRIVATE executor parser common storage test_utils)
    add_test(NAME project_operator_tests COMMAND test_project_operator)

    # 连接算子测试
    add_executable(test_join_operator test_join_operator.cpp)
    target_link_libraries(test_join_operator PRIVATE executor parser common storage test_utils)
    add_test(NAME join_operator_tests COMMAND test_join_operator)

    # 限制算子测试
    add_executable(test_limit_operator test_limit_operator.cpp)
    target_link_libraries(test_limit_operator PRIVATE executor parser common storage test_utils)
    add_test(NAME limit_operator_tests COMMAND test_limit_operator)

    # 排序算子测试
    add_executable(test_sort_operator test_sort_operator.cpp)
    target_link_libraries(test_sort_operator PRIVATE executor parser common storage test_utils)
    add_test(NAME sort_operator_tests COMMAND test_sort_operator)

    # 聚合算子测试
    add_executable(test_aggregate_operator test_aggregate_operator.cpp)
    target_link_libraries(test_aggregate_operator PRIVATE executor parser common storage test_utils)
    add_test(NAME aggregate_operator_tests COMMAND test_aggregate_operator)

    # DML执行器测试
    add_executable(test_dml_executor test_dml_executor.cpp)
    target_link_libraries(test_dml_executor PRIVATE executor parser common storage test_utils)
    add_test(NAME dml_executor_tests COMMAND test_dml_executor)

    # DDL执行器测试
    add_executable(test_ddl_executor test_ddl_executor.cpp)
    target_link_libraries(test_ddl_executor PRIVATE executor parser common storage test_utils)
    add_test(NAME ddl_executor_tests COMMAND test_ddl_executor)

    # 主执行器测试
    add_executable(test_executor test_executor.cpp)
    target_link_libraries(test_executor PRIVATE executor parser common storage Qt${QT_VERSION_MAJOR}::Core)
    add_test(NAME executor_tests COMMAND test_executor)

    # 统计信息测试
    add_executable(test_statistics test_statistics.cpp)
    target_link_libraries(test_statistics PRIVATE optimizer parser common storage Qt${QT_VERSION_MAJOR}::Core)
    add_test(NAME statistics_tests COMMAND test_statistics)

    # 索引选择器测试
    add_executable(test_index_selector test_index_selector.cpp)
    target_link_libraries(test_index_selector PRIVATE optimizer parser common storage Qt${QT_VERSION_MAJOR}::Core)
    add_test(NAME index_selector_tests COMMAND test_index_selector)

    # 查询重写器测试
    add_executable(test_query_rewriter test_query_rewriter.cpp)
    target_link_libraries(test_query_rewriter PRIVATE optimizer parser common Qt${QT_VERSION_MAJOR}::Core)
    add_test(NAME query_rewriter_tests COMMAND test_query_rewriter)

    # 规则优化器测试
    add_executable(test_rule_optimizer test_rule_optimizer.cpp)
    target_link_libraries(test_rule_optimizer PRIVATE optimizer executor parser common storage Qt${QT_VERSION_MAJOR}::Core)
    add_test(NAME rule_optimizer_tests COMMAND test_rule_optimizer)

    # 代价优化器测试
    add_executable(test_cost_optimizer test_cost_optimizer.cpp)
    target_link_libraries(test_cost_optimizer PRIVATE optimizer parser common storage Qt${QT_VERSION_MAJOR}::Core)
    add_test(NAME cost_optimizer_tests COMMAND test_cost_optimizer)

    # 计划生成器测试
    add_executable(test_plan_generator test_plan_generator.cpp)
    target_link_libraries(test_plan_generator PRIVATE optimizer executor parser common storage Qt${QT_VERSION_MAJOR}::Core)
    add_test(NAME plan_generator_tests COMMAND test_plan_generator)

    # EXPLAIN 处理器测试
    add_executable(test_explain test_explain.cpp)
    target_link_libraries(test_explain PRIVATE optimizer executor parser common storage Qt${QT_VERSION_MAJOR}::Core)
    add_test(NAME explain_tests COMMAND test_explain)

    # 子查询测试
    add_executable(test_subquery test_subquery.cpp)
    target_link_libraries(test_subquery PRIVATE executor optimizer parser common storage Qt${QT_VERSION_MAJOR}::Core)
    add_test(NAME subquery_tests COMMAND test_subquery)

    # 集成测试
    add_executable(test_integration test_integration.cpp)
    target_link_libraries(test_integration PRIVATE executor optimizer parser common storage Qt${QT_VERSION_MAJOR}::Core)
    add_test(NAME integration_tests COMMAND test_integration)

    # 外键约束测试
    add_executable(test_foreign_key test_foreign_key.cpp)
    target_link_libraries(test_foreign_key PRIVATE executor optimizer parser common storage Qt${QT_VERSION_MAJOR}::Core)
    add_test(NAME foreign_key_tests COMMAND test_foreign_key)

    # JOIN 执行测试 (Phase 7)
    add_executable(test_join test_join.cpp)
    target_link_libraries(test_join PRIVATE executor optimizer parser common storage Qt${QT_VERSION_MAJOR}::Core)
    add_test(NAME join_tests COMMAND test_join)

    # FROM 子查询测试 (Phase 7)
    add_executable(test_from_subquery test_from_subquery.cpp)
    target_link_libraries(test_from_subquery PRIVATE executor optimizer parser common storage Qt${QT_VERSION_MAJOR}::Core)
    add_test(NAME from_subquery_tests COMMAND test_from_subquery)

    # HAVING 子句测试 (Phase 7)
    add_executable(test_having test_having.cpp)
    target_link_libraries(test_having PRIVATE executor optimizer parser common storage Qt${QT_VERSION_MAJOR}::Core)
    add_test(NAME having_tests COMMAND test_having)
endif()

# ============================================
# 服务器测试 (仅Linux服务器版本)
# ============================================
if(BUILD_SERVER)
    # 基础测试（不依赖 Qt）
    add_executable(test_common test_common.cpp)
    target_link_libraries(test_common PRIVATE common test_utils)
    add_test(NAME common_tests COMMAND test_common)

    add_executable(test_storage test_storage.cpp)
    target_link_libraries(test_storage PRIVATE storage common server_concurrency test_utils)
    add_test(NAME storage_tests COMMAND test_storage)

    # 并发测试
    add_executable(test_concurrency ${CMAKE_CURRENT_SOURCE_DIR}/server/test_concurrency.cpp)
    target_link_libraries(test_concurrency PRIVATE
        server_concurrency
        common
        pthread
    )

    # 事务测试
    add_executable(test_transaction ${CMAKE_CURRENT_SOURCE_DIR}/server/test_transaction.cpp)
    target_link_libraries(test_transaction PRIVATE
        server_transaction
        common
        pthread
    )

    # 网络测试
    add_executable(test_network ${CMAKE_CURRENT_SOURCE_DIR}/server/test_network.cpp)
    target_link_libraries(test_network PRIVATE
        server_network
        common
        pthread
    )
endif()
