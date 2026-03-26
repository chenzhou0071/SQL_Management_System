#pragma once

#include "Token.h"
#include <string>
#include <vector>

namespace minisql {
namespace parser {

class Lexer {
public:
    explicit Lexer(const std::string& input);

    // 获取下一个 Token
    Token nextToken();

    // 浏览下一个 Token（不移动位置）
    Token peekToken();

    // 获取所有 Token
    std::vector<Token> getAllTokens();

    // 获取当前位置
    int getLine() const { return line_; }
    int getColumn() const { return column_; }

private:
    // 辅助方法
    char currentChar() const;
    char peekChar() const;
    void advance();
    void skipWhitespace();
    void skipComment();

    // Token 解析方法
    Token readNumber();
    Token readString(char quote);
    Token readIdentifierOrKeyword();
    Token readOperator();

    // 判断函数
    bool isAtEnd() const;
    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;

private:
    std::string input_;
    size_t pos_;
    int line_;
    int column_;
    Token peekedToken_;
    bool hasPeeked_;
};

}  // namespace parser
}  // namespace minisql
