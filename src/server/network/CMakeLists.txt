add_library(server_network STATIC)
target_sources(server_network PRIVATE
    SqlProtocol.cpp
    TcpConnection.cpp
    ThreadPool.cpp
    Reactor.cpp
    TcpServer.cpp
)
target_include_directories(server_network PUBLIC ${CMAKE_SOURCE_DIR}/src)
