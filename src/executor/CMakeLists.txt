add_library(executor
    ExpressionEvaluator.cpp
    ExecutionOperator.cpp
    TableScanOperator.cpp
    FilterOperator.cpp
    ProjectOperator.cpp
    LimitOperator.cpp
    SortOperator.cpp
    AggregateOperator.cpp
    DMLExecutor.cpp
    DDLExecutor.cpp
    Executor.cpp
    NestedLoopJoinOperator.cpp
    SubqueryScanOperator.cpp
)

target_include_directories(executor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/storage
    ${CMAKE_SOURCE_DIR}/src/optimizer
    ${CMAKE_SOURCE_DIR}/src/parser
)

target_link_libraries(executor PRIVATE common storage parser optimizer)
if(NOT BUILD_SERVER)
    target_link_libraries(executor PRIVATE Qt${QT_VERSION_MAJOR}::Core)
endif()
