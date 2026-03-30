cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# 静态链接libstdc++避免DLL版本冲突
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")

add_executable(minisql-client WIN32
    main.cpp
    mainwindow.cpp
    tcpclient.cpp
)

target_link_libraries(minisql-client
    PRIVATE
        Qt::Core
        Qt::Network
        Qt::Widgets
)
