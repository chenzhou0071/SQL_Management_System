add_library(server_transaction STATIC)
target_sources(server_transaction PRIVATE
    Transaction.cpp
    TransactionManager.cpp
    WALManager.cpp
)
target_include_directories(server_transaction PUBLIC ${CMAKE_SOURCE_DIR}/src)
