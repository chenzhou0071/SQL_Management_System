add_library(transaction
    TransactionManager.cpp
    Operation.cpp
)

target_include_directories(transaction PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
)

target_link_libraries(transaction PRIVATE common Qt${QT_VERSION_MAJOR}::Core)
