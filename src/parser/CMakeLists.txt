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

if(NOT BUILD_SERVER)
    target_link_libraries(parser PRIVATE Qt${QT_VERSION_MAJOR}::Core)
endif()
