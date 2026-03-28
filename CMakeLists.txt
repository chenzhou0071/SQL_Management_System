cmake_minimum_required(VERSION 3.16)
project(MiniSQL VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 服务器版本选项
option(BUILD_SERVER "Build Linux server version" OFF)

# ============================================
# 输出目录设置
# ============================================
if(BUILD_SERVER)
    # Linux服务器: build_server/bin/
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_server/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_server/lib)
    message(STATUS "Building Server Version")
    message(STATUS "Output: ${CMAKE_SOURCE_DIR}/build_server/bin")
else()
    # Windows GUI版本: build/bin/
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

    # Qt 6.5.3
    set(QT_VERSION_MAJOR 6)
    set(CMAKE_PREFIX_PATH "E:/Qt/6.5.3/mingw_64/lib/cmake" CACHE PATH "Qt cmake prefix")
    find_package(Qt${QT_VERSION_MAJOR} 6.5.3 COMPONENTS Core Widgets Gui REQUIRED)
    message(STATUS "Building GUI Version")
    message(STATUS "Output: ${CMAKE_BINARY_DIR}/bin")
endif()

# 警告级别
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

# ============================================
# 公共模块 (服务器和GUI都需要的)
# ============================================
add_subdirectory(src/common)
add_subdirectory(src/storage)
add_subdirectory(src/parser)
add_subdirectory(src/executor)
add_subdirectory(src/optimizer)

# ============================================
# 根据平台/模式选择编译目标
# ============================================
if(BUILD_SERVER)
    # ========================================
    # Linux服务器版本
    # ========================================
    add_subdirectory(src/server)

    add_executable(minisql_server
        src/server/main_server.cpp
    )

    target_link_libraries(minisql_server
        PRIVATE
            common
            -Wl,--start-group
            storage
            parser
            executor
            optimizer
            server_concurrency
            server_transaction
            server_network
            -Wl,--end-group
            pthread
    )

    # 运行时创建目录
    add_custom_command(TARGET minisql_server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:minisql_server>/../data/wal
    )

else()
    # ========================================
    # Windows GUI版本
    # ========================================
    add_subdirectory(src/client)

    add_executable(minisql
        src/main.cpp
    )

    target_link_libraries(minisql
        PRIVATE
            common
            storage
            parser
            executor
            optimizer
            client_gui
            Qt${QT_VERSION_MAJOR}::Core
            Qt${QT_VERSION_MAJOR}::Widgets
            Qt${QT_VERSION_MAJOR}::Gui
    )

    # 运行时创建目录
    add_custom_command(TARGET minisql POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:minisql>/../data
            $<TARGET_FILE_DIR:minisql>/../logs
    )
endif()

# ============================================
# 测试
# ============================================
enable_testing()
add_subdirectory(tests)
