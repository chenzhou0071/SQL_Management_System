add_library(executor
    Executor.cpp
    Expression.cpp
    DDLExecutor.cpp
    DMLExecutor.cpp
)

target_include_directories(executor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/storage
)

target_link_libraries(executor PRIVATE common storage Qt${QT_VERSION_MAJOR}::Core)
