add_library(storage
    TableManager.cpp
    IndexManager.cpp
    BPlusTree.cpp
    FileIO.cpp
)

target_include_directories(storage PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src
    E:/mingw64/mingw64/include
)

target_link_libraries(storage PRIVATE common)

# 服务器模式下需要链接并发控制
if(BUILD_SERVER)
    target_link_libraries(storage PRIVATE server_concurrency)
endif()

# Windows 下需要链接 stdc++fs（C++17 文件系统）
if(WIN32)
    target_link_libraries(storage PRIVATE stdc++fs)
endif()
