# SQL 解析模块实现计划

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**目标:** 实现完整的 SQL 解析层，包括词法分析、语法分析和语义分析三大模块

**架构:** 采用经典的编译器前端架构 - Lexer 将 SQL 字符串转换为 Token 流，Parser 将 Token 流转换为 AST，Semantic Analyzer 进行语义检查。使用递归下降分析法实现 Parser。

**技术栈:** C++17, STL, Qt (用于字符串处理), Google Test (测试框架)

---

## 模块一：词法分析器 (Lexer)

### Task 1: Token 定义

**文件:**
- Create: `src/parser/Token.h`
- Create: `tests/test_lexer.cpp`

**Step 1: 编写 Token 类型枚举测试**

在 `tests/test_lexer.cpp` 中写入：

```cpp
#include <gtest/gtest.h>
#include "parser/Token.h"

namespace minisql {
namespace parser {

TEST(TokenTest, TokenTypeEnumExists) {
    TokenType type = TokenType::KEYWORD;
    EXPECT_EQ(static_cast<int>(type), 0);
}

TEST(TokenTest, TokenStructExists) {
    Token token;
    token.type = TokenType::IDENTIFIER;
    token.value = "test";
    token.line = 1;
    token.column = 1;
    EXPECT_EQ(token.value, "test");
}

}  // namespace parser
}  // namespace minisql
```

**Step 2: 运行测试验证失败**

运行: `cd build && cmake --build . && ./tests/test_lexer`
预期: FAIL - "parser/Token.h: No such file or directory"

**Step 3: 实现 Token 定义**

在 `src/parser/Token.h` 中写入：

```cpp
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
```

**Step 4: 运行测试验证通过**

运行: `cd build && cmake --build . && ./tests/test_lexer`
预期: PASS

**Step 5: 提交**

```bash
git add src/parser/Token.h tests/test_lexer.cpp
git commit -m "feat(parser): add Token type definitions"
```

---

### Task 2: Keywords 工具类实现

**文件:**
- Create: `src/parser/Keywords.cpp`
- Modify: `tests/test_lexer.cpp`

**Step 1: 编写 Keywords 测试**

在 `tests/test_lexer.cpp` 末尾添加：

```cpp
TEST(KeywordsTest, IsKeyword) {
    EXPECT_TRUE(Keywords::isKeyword("SELECT"));
    EXPECT_TRUE(Keywords::isKeyword("select"));
    EXPECT_TRUE(Keywords::isKeyword("FROM"));
    EXPECT_FALSE(Keywords::isKeyword("tablename"));
    EXPECT_FALSE(Keywords::isKeyword(""));
}

TEST(KeywordsTest, GetKeywordType) {
    EXPECT_EQ(Keywords::getKeywordType("SELECT"), TokenType::KEYWORD);
    EXPECT_EQ(Keywords::getKeywordType("INSERT"), TokenType::KEYWORD);
    EXPECT_EQ(Keywords::getKeywordType("unknown"), TokenType::UNKNOWN);
}
```

**Step 2: 运行测试验证失败**

运行: `cd build && cmake --build . && ./tests/test_lexer`
预期: FAIL - undefined reference to Keywords::isKeyword

**Step 3: 实现 Keywords 类**

在 `src/parser/Keywords.cpp` 中写入：

```cpp
#include "Token.h"
#include <algorithm>
#include <cctype>

namespace minisql {
namespace parser {

std::unordered_map<std::string, TokenType> Keywords::keywords_;
bool Keywords::initialized_ = false;

void Keywords::initKeywords() {
    if (initialized_) return;

    // DDL 关键字
    keywords_["CREATE"] = TokenType::KEYWORD;
    keywords_["ALTER"] = TokenType::KEYWORD;
    keywords_["DROP"] = TokenType::KEYWORD;
    keywords_["TRUNCATE"] = TokenType::KEYWORD;

    // DML 关键字
    keywords_["INSERT"] = TokenType::KEYWORD;
    keywords_["UPDATE"] = TokenType::KEYWORD;
    keywords_["DELETE"] = TokenType::KEYWORD;
    keywords_["SELECT"] = TokenType::KEYWORD;

    // DCL 关键字
    keywords_["GRANT"] = TokenType::KEYWORD;
    keywords_["REVOKE"] = TokenType::KEYWORD;

    // 查询相关
    keywords_["FROM"] = TokenType::KEYWORD;
    keywords_["WHERE"] = TokenType::KEYWORD;
    keywords_["AND"] = TokenType::KEYWORD;
    keywords_["OR"] = TokenType::KEYWORD;
    keywords_["NOT"] = TokenType::KEYWORD;
    keywords_["IN"] = TokenType::KEYWORD;
    keywords_["LIKE"] = TokenType::KEYWORD;
    keywords_["BETWEEN"] = TokenType::KEYWORD;
    keywords_["IS"] = TokenType::KEYWORD;
    keywords_["NULL"] = TokenType::KEYWORD;
    keywords_["AS"] = TokenType::KEYWORD;
    keywords_["DISTINCT"] = TokenType::KEYWORD;

    // 排序和分组
    keywords_["ORDER"] = TokenType::KEYWORD;
    keywords_["BY"] = TokenType::KEYWORD;
    keywords_["ASC"] = TokenType::KEYWORD;
    keywords_["DESC"] = TokenType::KEYWORD;
    keywords_["GROUP"] = TokenType::KEYWORD;
    keywords_["HAVING"] = TokenType::KEYWORD;
    keywords_["LIMIT"] = TokenType::KEYWORD;
    keywords_["OFFSET"] = TokenType::KEYWORD;

    // 连接
    keywords_["JOIN"] = TokenType::KEYWORD;
    keywords_["INNER"] = TokenType::KEYWORD;
    keywords_["LEFT"] = TokenType::KEYWORD;
    keywords_["RIGHT"] = TokenType::KEYWORD;
    keywords_["FULL"] = TokenType::KEYWORD;
    keywords_["OUTER"] = TokenType::KEYWORD;
    keywords_["CROSS"] = TokenType::KEYWORD;
    keywords_["ON"] = TokenType::KEYWORD;

    // 表和数据库
    keywords_["TABLE"] = TokenType::KEYWORD;
    keywords_["DATABASE"] = TokenType::KEYWORD;
    keywords_["INDEX"] = TokenType::KEYWORD;
    keywords_["VIEW"] = TokenType::KEYWORD;
    keywords_["PROCEDURE"] = TokenType::KEYWORD;
    keywords_["TRIGGER"] = TokenType::KEYWORD;

    // 数据类型
    keywords_["INT"] = TokenType::KEYWORD;
    keywords_["INTEGER"] = TokenType::KEYWORD;
    keywords_["BIGINT"] = TokenType::KEYWORD;
    keywords_["SMALLINT"] = TokenType::KEYWORD;
    keywords_["TINYINT"] = TokenType::KEYWORD;
    keywords_["FLOAT"] = TokenType::KEYWORD;
    keywords_["DOUBLE"] = TokenType::KEYWORD;
    keywords_["DECIMAL"] = TokenType::KEYWORD;
    keywords_["CHAR"] = TokenType::KEYWORD;
    keywords_["VARCHAR"] = TokenType::KEYWORD;
    keywords_["TEXT"] = TokenType::KEYWORD;
    keywords_["DATE"] = TokenType::KEYWORD;
    keywords_["TIME"] = TokenType::KEYWORD;
    keywords_["DATETIME"] = TokenType::KEYWORD;
    keywords_["TIMESTAMP"] = TokenType::KEYWORD;
    keywords_["BOOLEAN"] = TokenType::KEYWORD;
    keywords_["BOOL"] = TokenType::KEYWORD;

    // 约束
    keywords_["PRIMARY"] = TokenType::KEYWORD;
    keywords_["KEY"] = TokenType::KEYWORD;
    keywords_["UNIQUE"] = TokenType::KEYWORD;
    keywords_["FOREIGN"] = TokenType::KEYWORD;
    keywords_["REFERENCES"] = TokenType::KEYWORD;
    keywords_["CONSTRAINT"] = TokenType::KEYWORD;
    keywords_["DEFAULT"] = TokenType::KEYWORD;
    keywords_["AUTO_INCREMENT"] = TokenType::KEYWORD;
    keywords_["NOT"] = TokenType::KEYWORD;
    keywords_["NULL"] = TokenType::KEYWORD;

    // 其他
    keywords_["SET"] = TokenType::KEYWORD;
    keywords_["VALUES"] = TokenType::KEYWORD;
    keywords_["INTO"] = TokenType::KEYWORD;
    keywords_["USE"] = TokenType::KEYWORD;
    keywords_["SHOW"] = TokenType::KEYWORD;
    keywords_["DATABASES"] = TokenType::KEYWORD;
    keywords_["TABLES"] = TokenType::KEYWORD;
    keywords_["COLUMNS"] = TokenType::KEYWORD;
    keywords_["IF"] = TokenType::KEYWORD;
    keywords_["EXISTS"] = TokenType::KEYWORD;
    keywords_["ENGINE"] = TokenType::KEYWORD;
    keywords_["COMMENT"] = TokenType::KEYWORD;
    keywords_["TRUE"] = TokenType::KEYWORD;
    keywords_["FALSE"] = TokenType::KEYWORD;

    initialized_ = true;
}

bool Keywords::isKeyword(const std::string& str) {
    initKeywords();
    std::string upper = str;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    return keywords_.find(upper) != keywords_.end();
}

TokenType Keywords::getKeywordType(const std::string& str) {
    initKeywords();
    std::string upper = str;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    auto it = keywords_.find(upper);
    return it != keywords_.end() ? it->second : TokenType::UNKNOWN;
}

}  // namespace parser
}  // namespace minisql
```

**Step 4: 更新 CMakeLists.txt**

在 `src/parser/CMakeLists.txt` 中添加 Keywords.cpp：

```cmake
add_library(parser
    Token.cpp
    Keywords.cpp
    Lexer.cpp
    Parser.cpp
    AST.cpp
)

target_include_directories(parser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
)

target_link_libraries(parser PRIVATE common Qt${QT_VERSION_MAJOR}::Core)
```

**Step 5: 创建 Token.cpp 实现 toString**

在 `src/parser/Token.cpp` 中写入：

```cpp
#include "Token.h"
#include <sstream>

namespace minisql {
namespace parser {

std::string Token::toString() const {
    std::ostringstream oss;
    oss << "Token(";
    switch (type) {
        case TokenType::KEYWORD: oss << "KEYWORD"; break;
        case TokenType::IDENTIFIER: oss << "IDENTIFIER"; break;
        case TokenType::STRING_LITERAL: oss << "STRING_LITERAL"; break;
        case TokenType::INT_LITERAL: oss << "INT_LITERAL"; break;
        case TokenType::FLOAT_LITERAL: oss << "FLOAT_LITERAL"; break;
        case TokenType::BOOL_LITERAL: oss << "BOOL_LITERAL"; break;
        case TokenType::NULL_LITERAL: oss << "NULL_LITERAL"; break;
        case TokenType::OPERATOR: oss << "OPERATOR"; break;
        case TokenType::DELIMITER: oss << "DELIMITER"; break;
        case TokenType::DOT: oss << "DOT"; break;
        case TokenType::COMMA: oss << "COMMA"; break;
        case TokenType::SEMICOLON: oss << "SEMICOLON"; break;
        case TokenType::LEFT_PAREN: oss << "LEFT_PAREN"; break;
        case TokenType::RIGHT_PAREN: oss << "RIGHT_PAREN"; break;
        case TokenType::EOF_TOKEN: oss << "EOF_TOKEN"; break;
        default: oss << "UNKNOWN"; break;
    }
    oss << ", \"" << value << "\", line=" << line << ", col=" << column << ")";
    return oss.str();
}

}  // namespace parser
}  // namespace minisql
```

**Step 6: 运行测试验证通过**

运行: `cd build && cmake --build . && ./tests/test_lexer`
预期: PASS

**Step 7: 提交**

```bash
git add src/parser/Token.cpp src/parser/Keywords.cpp src/parser/CMakeLists.txt tests/test_lexer.cpp
git commit -m "feat(parser): implement Keywords utility class"
```

---

### Task 3: Lexer 类基础结构

**文件:**
- Create: `src/parser/Lexer.h`
- Modify: `src/parser/Lexer.cpp`
- Modify: `tests/test_lexer.cpp`

**Step 1: 编写 Lexer 基础测试**

在 `tests/test_lexer.cpp` 末尾添加：

```cpp
#include "parser/Lexer.h"

TEST(LexerTest, EmptyInput) {
    Lexer lexer("");
    Token token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::EOF_TOKEN);
}

TEST(LexerTest, SkipWhitespace) {
    Lexer lexer("   \t\n  ");
    Token token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::EOF_TOKEN);
}

TEST(LexerTest, SingleIdentifier) {
    Lexer lexer("tablename");
    Token token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::IDENTIFIER);
    EXPECT_EQ(token.value, "tablename");
}
```

**Step 2: 运行测试验证失败**

运行: `cd build && cmake --build . && ./tests/test_lexer`
预期: FAIL - Lexer 类未定义

**Step 3: 实现 Lexer 类头文件**

在 `src/parser/Lexer.h` 中写入：

```cpp
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

    // ��览下一个 Token（不移动位置）
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
```

**Step 4: 实现 Lexer 基础方法**

更新 `src/parser/Lexer.cpp`：

```cpp
#include "Lexer.h"
#include "../common/Error.h"
#include <cctype>

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
```

**Step 5: 运行测试验证通过**

运行: `cd build && cmake --build . && ./tests/test_lexer`
预期: PASS

**Step 6: 提交**

```bash
git add src/parser/Lexer.h src/parser/Lexer.cpp tests/test_lexer.cpp
git commit -m "feat(parser): implement Lexer basic structure"
```

---

### Task 4: Lexer 数字解析

**文件:**
- Modify: `src/parser/Lexer.cpp`
- Modify: `tests/test_lexer.cpp`

**Step 1: 编写数字解析测试**

在 `tests/test_lexer.cpp` 末尾添加：

```cpp
TEST(LexerTest, IntegerLiteral) {
    Lexer lexer("12345");
    Token token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::INT_LITERAL);
    EXPECT_EQ(token.value, "12345");
}

TEST(LexerTest, FloatLiteral) {
    Lexer lexer("3.14159");
    Token token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::FLOAT_LITERAL);
    EXPECT_EQ(token.value, "3.14159");
}

TEST(LexerTest, FloatWithExponent) {
    Lexer lexer("1.5e10");
    Token token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::FLOAT_LITERAL);
    EXPECT_EQ(token.value, "1.5e10");
}

TEST(LexerTest, NegativeNumber) {
    Lexer lexer("-123");
    Token token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::OPERATOR);
    EXPECT_EQ(token.value, "-");
    token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::INT_LITERAL);
    EXPECT_EQ(token.value, "123");
}
```

**Step 2: 实现 readNumber 方法**

在 `src/parser/Lexer.cpp` 中的 Lexer 类添加：

```cpp
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
        if (currentChar() == 'e' || currentChar() == 'E') {
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
```

**Step 3: 运行测试验证通过**

运行: `cd build && cmake --build . && ./tests/test_lexer`
预期: PASS

**Step 4: 提交**

```bash
git add src/parser/Lexer.cpp tests/test_lexer.cpp
git commit -m "feat(parser): implement number parsing in Lexer"
```

---

### Task 5: Lexer 字符串解析

**文件:**
- Modify: `src/parser/Lexer.cpp`
- Modify: `tests/test_lexer.cpp`

**Step 1: 编写字符串解析测试**

在 `tests/test_lexer.cpp` 末尾添加：

```cpp
TEST(LexerTest, SingleQuotedString) {
    Lexer lexer("'hello world'");
    Token token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::STRING_LITERAL);
    EXPECT_EQ(token.value, "hello world");
}

TEST(LexerTest, DoubleQuotedString) {
    Lexer lexer("\"test string\"");
    Token token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::STRING_LITERAL);
    EXPECT_EQ(token.value, "test string");
}

TEST(LexerTest, StringWithEscape) {
    Lexer lexer("'it\\'s a test'");
    Token token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::STRING_LITERAL);
    EXPECT_EQ(token.value, "it's a test");
}

TEST(LexerTest, UnterminatedString) {
    Lexer lexer("'unterminated");
    EXPECT_THROW(lexer.nextToken(), minisql::MiniSQLException);
}
```

**Step 2: 实现 readString 方法**

在 `src/parser/Lexer.cpp` 中添加：

```cpp
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
```

**Step 3: 运行测试验证通过**

运行: `cd build && cmake --build . && ./tests/test_lexer`
预期: PASS

**Step 4: 提交**

```bash
git add src/parser/Lexer.cpp tests/test_lexer.cpp
git commit -m "feat(parser): implement string parsing in Lexer"
```

---

### Task 6: Lexer 标识符和关键字解析

**文件:**
- Modify: `src/parser/Lexer.cpp`
- Modify: `tests/test_lexer.cpp`

**Step 1: 编写标识符和关键字测试**

在 `tests/test_lexer.cpp` 末尾添加：

```cpp
TEST(LexerTest, Keyword) {
    Lexer lexer("SELECT");
    Token token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::KEYWORD);
    EXPECT_EQ(token.value, "SELECT");
}

TEST(LexerTest, KeywordCaseInsensitive) {
    Lexer lexer("select FROM where");
    Token token1 = lexer.nextToken();
    EXPECT_EQ(token1.type, TokenType::KEYWORD);
    EXPECT_EQ(token1.value, "SELECT");

    Token token2 = lexer.nextToken();
    EXPECT_EQ(token2.type, TokenType::KEYWORD);
    EXPECT_EQ(token2.value, "FROM");

    Token token3 = lexer.nextToken();
    EXPECT_EQ(token3.type, TokenType::KEYWORD);
    EXPECT_EQ(token3.value, "WHERE");
}

TEST(LexerTest, Identifier) {
    Lexer lexer("users");
    Token token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::IDENTIFIER);
    EXPECT_EQ(token.value, "users");
}

TEST(LexerTest, IdentifierWithUnderscore) {
    Lexer lexer("user_name");
    Token token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::IDENTIFIER);
    EXPECT_EQ(token.value, "user_name");
}
```

**Step 2: 实现 readIdentifierOrKeyword 方法**

在 `src/parser/Lexer.cpp` 中添加：

```cpp
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
```

**Step 3: 运行测试验证通过**

运行: `cd build && cmake --build . && ./tests/test_lexer`
预期: PASS

**Step 4: 提交**

```bash
git add src/parser/Lexer.cpp tests/test_lexer.cpp
git commit -m "feat(parser): implement identifier and keyword parsing"
```

---

### Task 7: Lexer 操作符解析

**文件:**
- Modify: `src/parser/Lexer.cpp`
- Modify: `tests/test_lexer.cpp`

**Step 1: 编写操作符测试**

在 `tests/test_lexer.cpp` 末尾添加：

```cpp
TEST(LexerTest, Operators) {
    Lexer lexer("= < > <= >= <> != + - * / %");
    Token token;

    token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::OPERATOR);
    EXPECT_EQ(token.value, "=");

    token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::OPERATOR);
    EXPECT_EQ(token.value, "<");

    token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::OPERATOR);
    EXPECT_EQ(token.value, ">");

    token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::OPERATOR);
    EXPECT_EQ(token.value, "<=");

    token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::OPERATOR);
    EXPECT_EQ(token.value, ">=");

    token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::OPERATOR);
    EXPECT_EQ(token.value, "<>");

    token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::OPERATOR);
    EXPECT_EQ(token.value, "!=");

    token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::OPERATOR);
    EXPECT_EQ(token.value, "+");

    token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::OPERATOR);
    EXPECT_EQ(token.value, "-");

    token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::OPERATOR);
    EXPECT_EQ(token.value, "*");

    token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::OPERATOR);
    EXPECT_EQ(token.value, "/");

    token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::OPERATOR);
    EXPECT_EQ(token.value, "%");
}
```

**Step 2: 实现 readOperator 方法**

在 `src/parser/Lexer.cpp` 中添加：

```cpp
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
            twoChar == "!=" || twoChar == "==" /* 用于未来支持 */) {
            advance();
            return Token(TokenType::OPERATOR, twoChar, startLine, startColumn);
        }
    }

    return Token(TokenType::OPERATOR, value, startLine, startColumn);
}
```

**Step 3: 运行测试验证通过**

运行: `cd build && cmake --build . && ./tests/test_lexer`
预期: PASS

**Step 4: 提交**

```bash
git add src/parser/Lexer.cpp tests/test_lexer.cpp
git commit -m "feat(parser): implement operator parsing in Lexer"
```

---

### Task 8: Lexer 注释处理

**文件:**
- Modify: `src/parser/Lexer.cpp`
- Modify: `tests/test_lexer.cpp`

**Step 1: 编写注释测试**

在 `tests/test_lexer.cpp` 末尾添加：

```cpp
TEST(LexerTest, SingleLineComment) {
    Lexer lexer("-- this is a comment\nSELECT");
    Token token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::KEYWORD);
    EXPECT_EQ(token.value, "SELECT");
}

TEST(LexerTest, MultiLineComment) {
    Lexer lexer("/* this is\n   a comment */ SELECT");
    Token token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::KEYWORD);
    EXPECT_EQ(token.value, "SELECT");
}

TEST(LexerTest, CommentAtEnd) {
    Lexer lexer("SELECT -- comment");
    Token token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::KEYWORD);
    EXPECT_EQ(token.value, "SELECT");
    token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::EOF_TOKEN);
}
```

**Step 2: 实现 skipComment 方法**

在 `src/parser/Lexer.cpp` 中添加：

```cpp
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
```

**Step 3: 运行测试验证通过**

运行: `cd build && cmake --build . && ./tests/test_lexer`
预期: PASS

**Step 4: 提交**

```bash
git add src/parser/Lexer.cpp tests/test_lexer.cpp
git commit -m "feat(parser): implement comment handling in Lexer"
```

---

### Task 9: Lexer 完整 SQL 语句测试

**文件:**
- Modify: `tests/test_lexer.cpp`

**Step 1: 编写完整 SQL 测试**

在 `tests/test_lexer.cpp` 末尾添加：

```cpp
TEST(LexerTest, SelectStatement) {
    Lexer lexer("SELECT id, name FROM users WHERE age > 18;");

    std::vector<Token> expected = {
        Token(TokenType::KEYWORD, "SELECT", 1, 1),
        Token(TokenType::IDENTIFIER, "id", 1, 8),
        Token(TokenType::COMMA, ",", 1, 10),
        Token(TokenType::IDENTIFIER, "name", 1, 12),
        Token(TokenType::KEYWORD, "FROM", 1, 17),
        Token(TokenType::IDENTIFIER, "users", 1, 22),
        Token(TokenType::KEYWORD, "WHERE", 1, 28),
        Token(TokenType::IDENTIFIER, "age", 1, 34),
        Token(TokenType::OPERATOR, ">", 1, 38),
        Token(TokenType::INT_LITERAL, "18", 1, 40),
        Token(TokenType::SEMICOLON, ";", 1, 42),
        Token(TokenType::EOF_TOKEN, "", 1, 43)
    };

    auto tokens = lexer.getAllTokens();
    ASSERT_EQ(tokens.size(), expected.size());
    for (size_t i = 0; i < tokens.size(); ++i) {
        EXPECT_EQ(tokens[i].type, expected[i].type) << "Token " << i;
        EXPECT_EQ(tokens[i].value, expected[i].value) << "Token " << i;
    }
}

TEST(LexerTest, InsertStatement) {
    Lexer lexer("INSERT INTO users (id, name) VALUES (1, 'Alice');");

    auto tokens = lexer.getAllTokens();
    EXPECT_EQ(tokens[0].type, TokenType::KEYWORD);
    EXPECT_EQ(tokens[0].value, "INSERT");
    EXPECT_EQ(tokens[1].type, TokenType::KEYWORD);
    EXPECT_EQ(tokens[1].value, "INTO");
    EXPECT_EQ(tokens[2].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[2].value, "users");
}
```

**Step 2: 运行测试验证通过**

运行: `cd build && cmake --build . && ./tests/test_lexer`
预期: PASS

**Step 3: 提交**

```bash
git add tests/test_lexer.cpp
git commit -m "test(parser): add complete SQL statement tests for Lexer"
```

---

## 模块二：AST 节点定义

### Task 10: AST 基础节点定义

**文件:**
- Create: `src/parser/AST.h`
- Modify: `src/parser/AST.cpp`
- Create: `tests/test_ast.cpp`

**Step 1: 编写 AST 基础测试**

在 `tests/test_ast.cpp` 中写入：

```cpp
#include <gtest/gtest.h>
#include "parser/AST.h"

namespace minisql {
namespace parser {

TEST(ASTTest, NodeBaseExists) {
    ASTNode node;
    EXPECT_EQ(node.getType(), ASTNodeType::UNKNOWN);
}

TEST(ASTTest, ExpressionNodeExists) {
    Expression expr;
    EXPECT_EQ(expr.getType(), ASTNodeType::EXPRESSION);
}

}  // namespace parser
}  // namespace minisql
```

**Step 2: 运行测试验证失败**

运行: `cd build && cmake --build . && ./tests/test_ast`
预期: FAIL - AST.h 不存在

**Step 3: 实现 AST 基础定义**

在 `src/parser/AST.h` 中写入：

```cpp
#pragma once

#include "Token.h"
#include "../common/Value.h"
#include <string>
#include <vector>
#include <memory>

namespace minisql {
namespace parser {

// ============================================================
// AST 节点类型枚举
// ============================================================
enum class ASTNodeType {
    UNKNOWN,

    // 语句节点
    SELECT_STMT,
    INSERT_STMT,
    UPDATE_STMT,
    DELETE_STMT,
    CREATE_STMT,
    DROP_STMT,
    ALTER_STMT,
    USE_STMT,
    SHOW_STMT,

    // 表达式节点
    EXPRESSION,
    BINARY_EXPR,
    UNARY_EXPR,
    LITERAL_EXPR,
    COLUMN_REF,
    FUNCTION_CALL,

    // 子句节点
    SELECT_CLAUSE,
    FROM_CLAUSE,
    WHERE_CLAUSE,
    GROUP_BY_CLAUSE,
    HAVING_CLAUSE,
    ORDER_BY_CLAUSE,
    LIMIT_CLAUSE,

    // 表引用
    TABLE_REF,
    JOIN_CLAUSE,

    // 列定义
    COLUMN_DEF,
    CONSTRAINT_DEF
};

// ============================================================
// AST 节点基类
// ============================================================
class ASTNode {
public:
    ASTNode(ASTNodeType type = ASTNodeType::UNKNOWN) : type_(type) {}
    virtual ~ASTNode() = default;

    ASTNodeType getType() const { return type_; }
    virtual std::string toString() const { return "ASTNode"; }

protected:
    ASTNodeType type_;
};

// ============================================================
// 表达式基类
// ============================================================
class Expression : public ASTNode {
public:
    Expression() : ASTNode(ASTNodeType::EXPRESSION) {}

    virtual std::string toString() const override { return "Expression"; }
};

using ExprPtr = std::shared_ptr<Expression>;

// ============================================================
// 字面量表达式
// ============================================================
class LiteralExpr : public Expression {
public:
    explicit LiteralExpr(const Value& value)
        : value_(value) {
        type_ = ASTNodeType::LITERAL_EXPR;
    }

    const Value& getValue() const { return value_; }
    std::string toString() const override {
        return value_.toString();
    }

private:
    Value value_;
};

// ============================================================
// 列引用表达式
// ============================================================
class ColumnRef : public Expression {
public:
    ColumnRef(const std::string& table, const std::string& column)
        : table_(table), column_(column) {
        type_ = ASTNodeType::COLUMN_REF;
    }

    const std::string& getTable() const { return table_; }
    const std::string& getColumn() const { return column_; }
    std::string toString() const override {
        if (table_.empty()) return column_;
        return table_ + "." + column_;
    }

private:
    std::string table_;
    std::string column_;
};

// ============================================================
// 二元表达式
// ============================================================
class BinaryExpr : public Expression {
public:
    BinaryExpr(ExprPtr left, const std::string& op, ExprPtr right)
        : left_(left), op_(op), right_(right) {
        type_ = ASTNodeType::BINARY_EXPR;
    }

    ExprPtr getLeft() const { return left_; }
    const std::string& getOp() const { return op_; }
    ExprPtr getRight() const { return right_; }
    std::string toString() const override {
        return "(" + left_->toString() + " " + op_ + " " + right_->toString() + ")";
    }

private:
    ExprPtr left_;
    std::string op_;
    ExprPtr right_;
};

// ============================================================
// 一元表达式
// ============================================================
class UnaryExpr : public Expression {
public:
    UnaryExpr(const std::string& op, ExprPtr operand)
        : op_(op), operand_(operand) {
        type_ = ASTNodeType::UNARY_EXPR;
    }

    const std::string& getOp() const { return op_; }
    ExprPtr getOperand() const { return operand_; }
    std::string toString() const override {
        return op_ + operand_->toString();
    }

private:
    std::string op_;
    ExprPtr operand_;
};

}  // namespace parser
}  // namespace minisql
```

**Step 4: 更新 CMakeLists.txt**

在 `src/parser/CMakeLists.txt` 确认包含 AST.cpp

**Step 5: 运行测试验证通过**

运行: `cd build && cmake --build . && ./tests/test_ast`
预期: PASS

**Step 6: 提交**

```bash
git add src/parser/AST.h tests/test_ast.cpp
git commit -m "feat(parser): add AST node base definitions"
```

---

### Task 11: SQL 语句 AST 节点

**文件:**
- Modify: `src/parser/AST.h`
- Modify: `tests/test_ast.cpp`

**Step 1: 编写语句节点测试**

在 `tests/test_ast.cpp` 末尾添加：

```cpp
TEST(ASTTest, SelectStmtExists) {
    SelectStmt stmt;
    EXPECT_EQ(stmt.getType(), ASTNodeType::SELECT_STMT);
}

TEST(ASTTest, InsertStmtExists) {
    InsertStmt stmt;
    EXPECT_EQ(stmt.getType(), ASTNodeType::INSERT_STMT);
}
```

**Step 2: 实现语句节点**

在 `src/parser/AST.h` 末尾添加：

```cpp
// ============================================================
// 表引用
// ============================================================
struct TableRef : public ASTNode {
    std::string name;
    std::string alias;
    std::string database;

    TableRef() : ASTNode(ASTNodeType::TABLE_REF) {}
    std::string toString() const override {
        if (!alias.empty()) return name + " AS " + alias;
        return name;
    }
};

// ============================================================
// JOIN 子句
// ============================================================
struct JoinClause : public ASTNode {
    std::string joinType;  // INNER, LEFT, RIGHT, FULL
    std::shared_ptr<TableRef> table;
    ExprPtr onCondition;

    JoinClause() : ASTNode(ASTNodeType::JOIN_CLAUSE) {}
};

// ============================================================
// ORDER BY 项
// ============================================================
struct OrderByItem {
    ExprPtr expr;
    bool ascending = true;
};

// ============================================================
// SELECT 语句
// ============================================================
class SelectStmt : public ASTNode {
public:
    SelectStmt() : ASTNode(ASTNodeType::SELECT_STMT), distinct(false), limit(-1), offset(-1) {}

    // SELECT 子句
    std::vector<ExprPtr> selectItems;
    bool distinct;

    // FROM 子句
    std::shared_ptr<TableRef> fromTable;
    std::vector<std::shared_ptr<JoinClause>> joins;

    // WHERE 子句
    ExprPtr whereClause;

    // GROUP BY 子句
    std::vector<ExprPtr> groupBy;

    // HAVING 子句
    ExprPtr havingClause;

    // ORDER BY 子句
    std::vector<OrderByItem> orderBy;

    // LIMIT 子句
    int limit;
    int offset;

    std::string toString() const override {
        return "SELECT ...";
    }
};

// ============================================================
// INSERT 语句
// ============================================================
class InsertStmt : public ASTNode {
public:
    InsertStmt() : ASTNode(ASTNodeType::INSERT_STMT) {}

    std::string table;
    std::vector<std::string> columns;
    std::vector<std::vector<ExprPtr>> valuesList;  // 支持批量插入

    std::string toString() const override {
        return "INSERT INTO " + table + " ...";
    }
};

// ============================================================
// UPDATE 语句
// ============================================================
class UpdateStmt : public ASTNode {
public:
    UpdateStmt() : ASTNode(ASTNodeType::UPDATE_STMT) {}

    std::string table;
    std::vector<std::pair<std::string, ExprPtr>> assignments;
    ExprPtr whereClause;

    std::string toString() const override {
        return "UPDATE " + table + " ...";
    }
};

// ============================================================
// DELETE 语句
// ============================================================
class DeleteStmt : public ASTNode {
public:
    DeleteStmt() : ASTNode(ASTNodeType::DELETE_STMT) {}

    std::string table;
    ExprPtr whereClause;

    std::string toString() const override {
        return "DELETE FROM " + table + " ...";
    }
};

// ============================================================
// CREATE TABLE 语句
// ============================================================
struct ColumnDefNode : public ASTNode {
    std::string name;
    DataType type;
    int length = 0;
    bool notNull = false;
    bool primaryKey = false;
    bool unique = false;
    bool autoIncrement = false;
    std::string defaultValue;

    ColumnDefNode() : ASTNode(ASTNodeType::COLUMN_DEF) {}
};

struct ConstraintDefNode : public ASTNode {
    ConstraintType type;
    std::string name;
    std::vector<std::string> columns;

    ConstraintDefNode() : ASTNode(ASTNodeType::CONSTRAINT_DEF) {}
};

class CreateTableStmt : public ASTNode {
public:
    CreateTableStmt() : ASTNode(ASTNodeType::CREATE_STMT), ifNotExists(false) {}

    std::string table;
    std::vector<std::shared_ptr<ColumnDefNode>> columns;
    std::vector<std::shared_ptr<ConstraintDefNode>> constraints;
    bool ifNotExists;
    std::string engine;
    std::string comment;

    std::string toString() const override {
        return "CREATE TABLE " + table + " ...";
    }
};

// ============================================================
// DROP 语句
// ============================================================
class DropStmt : public ASTNode {
public:
    DropStmt() : ASTNode(ASTNodeType::DROP_STMT), ifExists(false) {}

    std::string objectType;  // TABLE, DATABASE, INDEX, VIEW
    std::string name;
    bool ifExists;

    std::string toString() const override {
        return "DROP " + objectType + " " + name;
    }
};

// ============================================================
// USE 语句
// ============================================================
class UseStmt : public ASTNode {
public:
    UseStmt() : ASTNode(ASTNodeType::USE_STMT) {}

    std::string database;

    std::string toString() const override {
        return "USE " + database;
    }
};

// ============================================================
// SHOW 语句
// ============================================================
class ShowStmt : public ASTNode {
public:
    ShowStmt() : ASTNode(ASTNodeType::SHOW_STMT) {}

    std::string objectType;  // DATABASES, TABLES, COLUMNS, INDEX
    std::string database;
    std::string table;

    std::string toString() const override {
        return "SHOW " + objectType;
    }
};
```

**Step 3: 运行测试验证通过**

运行: `cd build && cmake --build . && ./tests/test_ast`
预期: PASS

**Step 4: 提交**

```bash
git add src/parser/AST.h tests/test_ast.cpp
git commit -m "feat(parser): add SQL statement AST nodes"
```

---

## 模块三：语法分析器 (Parser)

### Task 12: Parser 基础结构

**文件:**
- Create: `src/parser/Parser.h`
- Modify: `src/parser/Parser.cpp`
- Create: `tests/test_parser.cpp`

**Step 1: 编写 Parser 基础测试**

在 `tests/test_parser.cpp` 中写入：

```cpp
#include <gtest/gtest.h>
#include "parser/Parser.h"
#include "parser/Lexer.h"

namespace minisql {
namespace parser {

TEST(ParserTest, EmptyInput) {
    Lexer lexer("");
    Parser parser(lexer);
    EXPECT_THROW(parser.parseStatement(), MiniSQLException);
}

TEST(ParserTest, ParseUseStatement) {
    Lexer lexer("USE testdb;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    EXPECT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->getType(), ASTNodeType::USE_STMT);
}

}  // namespace parser
}  // namespace minisql
```

**Step 2: 实现 Parser 头文件**

在 `src/parser/Parser.h` 中写入：

```cpp
#pragma once

#include "Lexer.h"
#include "AST.h"
#include <memory>

namespace minisql {
namespace parser {

class Parser {
public:
    explicit Parser(Lexer& lexer);

    // 解析 SQL 语句
    std::shared_ptr<ASTNode> parseStatement();

private:
    // 辅助方法
    Token currentToken() const { return current_; }
    Token peekToken() { return lexer_.peekToken(); }
    void advance() { current_ = lexer_.nextToken(); }
    bool check(TokenType type) const { return current_.type == type; }
    bool checkKeyword(const std::string& keyword) const;
    bool match(TokenType type);
    bool matchKeyword(const std::string& keyword);
    void expect(TokenType type, const std::string& message);
    void expectKeyword(const std::string& keyword, const std::string& message);

    // 解析语句
    std::shared_ptr<ASTNode> parseUseStatement();
    std::shared_ptr<ASTNode> parseShowStatement();
    std::shared_ptr<SelectStmt> parseSelectStatement();
    std::shared_ptr<InsertStmt> parseInsertStatement();
    std::shared_ptr<UpdateStmt> parseUpdateStatement();
    std::shared_ptr<DeleteStmt> parseDeleteStatement();
    std::shared_ptr<CreateTableStmt> parseCreateStatement();
    std::shared_ptr<DropStmt> parseDropStatement();

    // 解析表达式
    ExprPtr parseExpression();
    ExprPtr parseOrExpression();
    ExprPtr parseAndExpression();
    ExprPtr parseNotExpression();
    ExprPtr parseComparisonExpression();
    ExprPtr parseAdditiveExpression();
    ExprPtr parseMultiplicativeExpression();
    ExprPtr parseUnaryExpression();
    ExprPtr parsePrimaryExpression();

    // 解析表引用
    std::shared_ptr<TableRef> parseTableRef();
    std::shared_ptr<JoinClause> parseJoinClause();

    // 解析列表
    std::vector<ExprPtr> parseExpressionList();
    std::vector<std::string> parseIdentifierList();

private:
    Lexer& lexer_;
    Token current_;
};

}  // namespace parser
}  // namespace minisql
```

**Step 3: 实现 Parser 基础方法**

更新 `src/parser/Parser.cpp`：

```cpp
#include "Parser.h"
#include "../common/Error.h"

namespace minisql {
namespace parser {

Parser::Parser(Lexer& lexer)
    : lexer_(lexer), current_(TokenType::UNKNOWN, "", 0, 0) {
    advance();  // 预读第一个 token
}

bool Parser::checkKeyword(const std::string& keyword) const {
    return current_.type == TokenType::KEYWORD && current_.value == keyword;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::matchKeyword(const std::string& keyword) {
    if (checkKeyword(keyword)) {
        advance();
        return true;
    }
    return false;
}

void Parser::expect(TokenType type, const std::string& message) {
    if (!check(type)) {
        throw MiniSQLException(
            ErrorCode::PARSER_MISSING_TOKEN,
            message + ", got: " + current_.toString()
        );
    }
    advance();
}

void Parser::expectKeyword(const std::string& keyword, const std::string& message) {
    if (!checkKeyword(keyword)) {
        throw MiniSQLException(
            ErrorCode::PARSER_MISSING_TOKEN,
            message + ", got: " + current_.toString()
        );
    }
    advance();
}

std::shared_ptr<ASTNode> Parser::parseStatement() {
    if (current_.type == TokenType::EOF_TOKEN) {
        throw MiniSQLException(ErrorCode::PARSER_SYNTAX_ERROR, "Unexpected end of input");
    }

    if (checkKeyword("USE")) return parseUseStatement();
    if (checkKeyword("SHOW")) return parseShowStatement();
    if (checkKeyword("SELECT")) return parseSelectStatement();
    if (checkKeyword("INSERT")) return parseInsertStatement();
    if (checkKeyword("UPDATE")) return parseUpdateStatement();
    if (checkKeyword("DELETE")) return parseDeleteStatement();
    if (checkKeyword("CREATE")) return parseCreateStatement();
    if (checkKeyword("DROP")) return parseDropStatement();

    throw MiniSQLException(
        ErrorCode::PARSER_SYNTAX_ERROR,
        "Unknown statement: " + current_.value
    );
}

std::shared_ptr<ASTNode> Parser::parseUseStatement() {
    expectKeyword("USE", "Expected USE");

    if (!check(TokenType::IDENTIFIER)) {
        throw MiniSQLException(
            ErrorCode::PARSER_MISSING_TOKEN,
            "Expected database name after USE"
        );
    }

    auto stmt = std::make_shared<UseStmt>();
    stmt->database = current_.value;
    advance();

    match(TokenType::SEMICOLON);  // 可选的分号

    return stmt;
}

std::shared_ptr<ASTNode> Parser::parseShowStatement() {
    expectKeyword("SHOW", "Expected SHOW");

    auto stmt = std::make_shared<ShowStmt>();

    if (checkKeyword("DATABASES")) {
        stmt->objectType = "DATABASES";
        advance();
    } else if (checkKeyword("TABLES")) {
        stmt->objectType = "TABLES";
        advance();
    } else {
        throw MiniSQLException(
            ErrorCode::PARSER_SYNTAX_ERROR,
            "Expected DATABASES or TABLES after SHOW"
        );
    }

    match(TokenType::SEMICOLON);
    return stmt;
}

}  // namespace parser
}  // namespace minisql
```

**Step 4: 运行测试验证通过**

运行: `cd build && cmake --build . && ./tests/test_parser`
预期: PASS

**Step 5: 提交**

```bash
git add src/parser/Parser.h src/parser/Parser.cpp tests/test_parser.cpp
git commit -m "feat(parser): implement Parser basic structure"
```

---

**（由于篇幅限制，后续任务继续按照相同格式...）**

任务列表继续：
- Task 13: Parser SELECT 语句解析
- Task 14: Parser INSERT 语句解析
- Task 15: Parser UPDATE 语句解析
- Task 16: Parser DELETE 语句解析
- Task 17: Parser CREATE TABLE 语句解析
- Task 18: Parser DROP 语句解析
- Task 19: Parser 表达式解析
- Task 20: Parser JOIN 子句解析
- Task 21: Parser 完整测试

## 模块四：语义分析

### Task 22: SemanticAnalyzer 基础结构
### Task 23: 表存在性检查
### Task 24: 字段存在性检查
### Task 25: 类型检查
### Task 26: 约束检查
### Task 27: 集成测试

---

## 验收标准

- [ ] 所有单元测试通过
- [ ] Lexer 能正确分词各种 SQL 语句
- [ ] Parser 能正确解析 SELECT, INSERT, UPDATE, DELETE, CREATE, DROP 语句
- [ ] AST 结构清晰完整
- [ ] 语义分析能检查表、字段、类型、约束
- [ ] 代码覆盖率 > 80%
- [ ] 代码符合 C++17 标准
- [ ] 无内存泄漏

---

## 测试命令

```bash
# 构建项目
cd build
cmake ..
make

# 运行所有测试
./tests/test_lexer
./tests/test_ast
./tests/test_parser
./tests/test_semantic

# 运行覆盖率测试
gcov *.cpp
```

---

## 预估时间

- 词法分析器: 5-7 天
- AST 定义: 2-3 天
- 语法分析器: 10-14 天
- 语义分析: 5-7 天
- 测试和调试: 5-7 天

**总计: 27-38 天 (约 3-4 周)**