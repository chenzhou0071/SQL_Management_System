cmake_minimum_required(VERSION 3.16)
project(MiniSQL VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt 6.5.3
set(QT_VERSION_MAJOR 6)
set(CMAKE_PREFIX_PATH "E:/Qt/6.5.3/mingw_64/lib/cmake" CACHE PATH "Qt cmake prefix")
find_package(Qt${QT_VERSION_MAJOR} 6.5.3 COMPONENTS Core Widgets Gui REQUIRED)

message(STATUS "Using Qt${QT_VERSION_MAJOR}")

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 警告级别
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

# 公共模块
add_subdirectory(common)

# 存储引擎
add_subdirectory(storage)

# 解析层
add_subdirectory(parser)

# 执行器
add_subdirectory(executor)

# 优化器
add_subdirectory(optimizer)

# 主程序
add_executable(minisql
    main.cpp
)

target_link_libraries(minisql
    PRIVATE
        common
        storage
        parser
        executor
        optimizer
        transaction
        application
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Gui
)

# 运行时创建 data 目录
add_custom_command(TARGET minisql POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:minisql>/../data
        $<TARGET_FILE_DIR:minisql>/../logs
)

# 测试
enable_testing()
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../tests tests)
