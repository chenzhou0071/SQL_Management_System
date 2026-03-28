add_library(server_concurrency STATIC)
target_sources(server_concurrency PRIVATE
    LockManager.cpp
)
target_include_directories(server_concurrency PUBLIC ${CMAKE_SOURCE_DIR}/src)
