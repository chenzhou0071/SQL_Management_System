add_library(optimizer
    Optimizer.cpp
    RuleOptimizer.cpp
    Explain.cpp
)

target_include_directories(optimizer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
)

target_link_libraries(optimizer PRIVATE common Qt${QT_VERSION_MAJOR}::Core)
