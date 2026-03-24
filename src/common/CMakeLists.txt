add_library(common
    Logger.cpp
    Type.cpp
    Error.cpp
    Value.cpp
)

target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# common 库不依赖 Qt，纯 C++ 实现
