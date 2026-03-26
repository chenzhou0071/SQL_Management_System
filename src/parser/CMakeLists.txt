add_library(parser
    Token.cpp
    Keywords.cpp
    Lexer.cpp
    Parser.cpp
    SemanticAnalyzer.cpp
)

target_include_directories(parser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
)

target_link_libraries(parser PRIVATE common storage Qt${QT_VERSION_MAJOR}::Core)
