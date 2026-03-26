#pragma once

#include <string>
#include <unordered_map>

namespace minisql {
namespace parser {

// ============================================================
// Token 类型枚举
// ============================================================
enum class TokenType {
    // 关键字
    KEYWORD,

    // 标识符
    IDENTIFIER,

    // 字面量
    STRING_LITERAL,
    INT_LITERAL,
    FLOAT_LITERAL,
    BOOL_LITERAL,
    NULL_LITERAL,

    // 操作符
    OPERATOR,        // =, >, <, >=, <=, <>, !=, +, -, *, /, %

    // 分隔符
    DELIMITER,       // , ; ( ) { }
    DOT,             // .

    // 特殊
    COMMA,           // ,
    SEMICOLON,       // ;
    LEFT_PAREN,      // (
    RIGHT_PAREN,     // )
    LEFT_BRACKET,    // [
    RIGHT_BRACKET,   // ]
    EOF_TOKEN,       // 结束标记

    // 注释
    COMMENT,

    // 未知
    UNKNOWN
};

// ============================================================
// Token 结构体
// ============================================================
struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;

    Token() : type(TokenType::UNKNOWN), line(0), column(0) {}
    Token(TokenType t, const std::string& v, int l = 0, int c = 0)
        : type(t), value(v), line(l), column(c) {}

    std::string toString() const;
};

// ============================================================
// 关键字表
// ============================================================
class Keywords {
public:
    static bool isKeyword(const std::string& str);
    static TokenType getKeywordType(const std::string& str);

private:
    static std::unordered_map<std::string, TokenType> keywords_;
    static bool initialized_;
    static void initKeywords();
};

}  // namespace parser
}  // namespace minisql
