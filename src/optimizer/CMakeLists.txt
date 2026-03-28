add_library(optimizer
    QueryOptimizer.cpp
    RuleOptimizer.cpp
    Statistics.cpp
    IndexSelector.cpp
    QueryRewriter.cpp
    CostOptimizer.cpp
    PlanGenerator.cpp
    ExplainHandler.cpp
)

target_include_directories(optimizer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/storage
    ${CMAKE_SOURCE_DIR}/src/parser
    ${CMAKE_SOURCE_DIR}/src/executor
)

target_link_libraries(optimizer PRIVATE common storage parser executor Qt${QT_VERSION_MAJOR}::Core)
