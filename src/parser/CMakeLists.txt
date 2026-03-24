add_library(parser
    Lexer.cpp
    Parser.cpp
    AST.cpp
)

target_include_directories(parser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
)

target_link_libraries(parser PRIVATE common Qt${QT_VERSION_MAJOR}::Core)
