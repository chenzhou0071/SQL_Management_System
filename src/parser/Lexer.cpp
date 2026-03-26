#include "Lexer.h"
#include "../common/Error.h"
#include <cctype>
#include <algorithm>

namespace minisql {
namespace parser {

Lexer::Lexer(const std::string& input)
    : input_(input), pos_(0), line_(1), column_(1), hasPeeked_(false) {
}

char Lexer::currentChar() const {
    if (isAtEnd()) return '\0';
    return input_[pos_];
}

char Lexer::peekChar() const {
    if (pos_ + 1 >= input_.size()) return '\0';
    return input_[pos_ + 1];
}

void Lexer::advance() {
    if (!isAtEnd()) {
        if (input_[pos_] == '\n') {
            line_++;
            column_ = 1;
        } else {
            column_++;
        }
        pos_++;
    }
}

void Lexer::skipWhitespace() {
    while (!isAtEnd() && std::isspace(currentChar())) {
        advance();
    }
}

void Lexer::skipComment() {
    while (!isAtEnd()) {
        // 单行注释 --
        if (currentChar() == '-' && peekChar() == '-') {
            advance();  // 跳过第一个 -
            advance();  // 跳过第二个 -
            // 跳过到行尾
            while (!isAtEnd() && currentChar() != '\n') {
                advance();
            }
            // skipWhitespace 会在下一次调用时处理换行
        }
        // 多行注释 /* */
        else if (currentChar() == '/' && peekChar() == '*') {
            advance();  // 跳过 /
            advance();  // 跳过 *
            // 跳过到 */
            while (!isAtEnd()) {
                if (currentChar() == '*' && peekChar() == '/') {
                    advance();  // 跳过 *
                    advance();  // 跳过 /
                    break;
                }
                advance();
            }
        }
        else {
            break;
        }
        skipWhitespace();
    }
}

bool Lexer::isAtEnd() const {
    return pos_ >= input_.size();
}

bool Lexer::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool Lexer::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}

Token Lexer::readNumber() {
    int startLine = line_;
    int startColumn = column_;
    std::string value;

    // 读取整数部分
    while (!isAtEnd() && isDigit(currentChar())) {
        value += currentChar();
        advance();
    }

    // 检查小数点
    if (currentChar() == '.' && isDigit(peekChar())) {
        value += currentChar();  // 添加小数点
        advance();

        // 读取小数部分
        while (!isAtEnd() && isDigit(currentChar())) {
            value += currentChar();
            advance();
        }

        // 检查指数部分
        if ((currentChar() == 'e' || currentChar() == 'E') &&
            (isDigit(peekChar()) || peekChar() == '+' || peekChar() == '-')) {
            value += currentChar();
            advance();

            if (currentChar() == '+' || currentChar() == '-') {
                value += currentChar();
                advance();
            }

            while (!isAtEnd() && isDigit(currentChar())) {
                value += currentChar();
                advance();
            }
        }

        return Token(TokenType::FLOAT_LITERAL, value, startLine, startColumn);
    }

    return Token(TokenType::INT_LITERAL, value, startLine, startColumn);
}

Token Lexer::readString(char quote) {
    int startLine = line_;
    int startColumn = column_ - 1;  // 包含开始的引号
    std::string value;

    while (!isAtEnd() && currentChar() != quote) {
        if (currentChar() == '\\') {
            advance();  // 跳过反斜杠
            if (isAtEnd()) {
                throw minisql::MiniSQLException(
                    minisql::ErrorCode::LEXER_UNTERMINATED_STRING,
                    "Unterminated string literal at line " + std::to_string(startLine)
                );
            }
            // 处理转义字符
            char escaped = currentChar();
            switch (escaped) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '\\': value += '\\'; break;
                case '\'': value += '\''; break;
                case '"': value += '"'; break;
                default: value += escaped; break;
            }
            advance();
        } else {
            value += currentChar();
            advance();
        }
    }

    if (isAtEnd()) {
        throw minisql::MiniSQLException(
            minisql::ErrorCode::LEXER_UNTERMINATED_STRING,
            "Unterminated string literal at line " + std::to_string(startLine)
        );
    }

    advance();  // 跳过结束引号
    return Token(TokenType::STRING_LITERAL, value, startLine, startColumn);
}

Token Lexer::readIdentifierOrKeyword() {
    int startLine = line_;
    int startColumn = column_;
    std::string value;

    while (!isAtEnd() && isAlphaNumeric(currentChar())) {
        value += currentChar();
        advance();
    }

    // 转换为大写进行关键字检查
    std::string upper = value;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    if (Keywords::isKeyword(upper)) {
        return Token(TokenType::KEYWORD, upper, startLine, startColumn);
    }

    return Token(TokenType::IDENTIFIER, value, startLine, startColumn);
}

Token Lexer::readOperator() {
    int startLine = line_;
    int startColumn = column_;
    char c = currentChar();
    std::string value(1, c);
    advance();

    // 检查两字符操作符
    if (!isAtEnd()) {
        char next = currentChar();
        std::string twoChar = value + next;

        if (twoChar == "<=" || twoChar == ">=" || twoChar == "<>" ||
            twoChar == "!=" || twoChar == "==") {
            advance();
            return Token(TokenType::OPERATOR, twoChar, startLine, startColumn);
        }
    }

    return Token(TokenType::OPERATOR, value, startLine, startColumn);
}

Token Lexer::nextToken() {
    if (hasPeeked_) {
        hasPeeked_ = false;
        return peekedToken_;
    }

    skipWhitespace();
    skipComment();

    if (isAtEnd()) {
        return Token(TokenType::EOF_TOKEN, "", line_, column_);
    }

    char c = currentChar();
    int startLine = line_;
    int startColumn = column_;

    // 数字
    if (isDigit(c)) {
        return readNumber();
    }

    // 字符串
    if (c == '\'' || c == '"') {
        advance();  // 跳过引号
        return readString(c);
    }

    // 标识符或关键字
    if (isAlpha(c)) {
        return readIdentifierOrKeyword();
    }

    // 操作符和分隔符
    switch (c) {
        case ',':
            advance();
            return Token(TokenType::COMMA, ",", startLine, startColumn);
        case ';':
            advance();
            return Token(TokenType::SEMICOLON, ";", startLine, startColumn);
        case '(':
            advance();
            return Token(TokenType::LEFT_PAREN, "(", startLine, startColumn);
        case ')':
            advance();
            return Token(TokenType::RIGHT_PAREN, ")", startLine, startColumn);
        case '.':
            advance();
            return Token(TokenType::DOT, ".", startLine, startColumn);
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
        case '=':
        case '<':
        case '>':
        case '!':
            return readOperator();
        default:
            advance();
            return Token(TokenType::UNKNOWN, std::string(1, c), startLine, startColumn);
    }
}

Token Lexer::peekToken() {
    if (!hasPeeked_) {
        peekedToken_ = nextToken();
        hasPeeked_ = true;
    }
    return peekedToken_;
}

std::vector<Token> Lexer::getAllTokens() {
    std::vector<Token> tokens;
    Token token;
    do {
        token = nextToken();
        tokens.push_back(token);
    } while (token.type != TokenType::EOF_TOKEN);
    return tokens;
}

}  // namespace parser
}  // namespace minisql
