#include "../src/parser/Token.h"
#include "../src/parser/Lexer.h"
#include "../src/common/Error.h"
#include "test_utils.h"
#include <iostream>

using namespace minisql;
using namespace parser;

// ============================================================
// Token 测试
// ============================================================

TEST(testTokenTypeEnumExists) {
    TokenType type = TokenType::KEYWORD;
    ASSERT_EQ(static_cast<int>(type), 0);
}

TEST(testTokenStructExists) {
    Token token;
    token.type = TokenType::IDENTIFIER;
    token.value = "test";
    token.line = 1;
    token.column = 1;
    ASSERT_EQ(token.value, "test");
}

TEST(testKeywordsIsKeyword) {
    ASSERT_TRUE(Keywords::isKeyword("SELECT"));
    ASSERT_TRUE(Keywords::isKeyword("select"));
    ASSERT_TRUE(Keywords::isKeyword("FROM"));
    ASSERT_FALSE(Keywords::isKeyword("tablename"));
    ASSERT_FALSE(Keywords::isKeyword(""));
}

TEST(testKeywordsGetKeywordType) {
    ASSERT_EQ(static_cast<int>(Keywords::getKeywordType("SELECT")), static_cast<int>(TokenType::KEYWORD));
    ASSERT_EQ(static_cast<int>(Keywords::getKeywordType("INSERT")), static_cast<int>(TokenType::KEYWORD));
    ASSERT_EQ(static_cast<int>(Keywords::getKeywordType("unknown")), static_cast<int>(TokenType::UNKNOWN));
}

TEST(testLexerEmptyInput) {
    Lexer lexer("");
    Token token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::EOF_TOKEN));
}

TEST(testLexerSkipWhitespace) {
    Lexer lexer("   \t\n  ");
    Token token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::EOF_TOKEN));
}

TEST(testLexerSingleIdentifier) {
    Lexer lexer("tablename");
    Token token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::IDENTIFIER));
    ASSERT_EQ(token.value, "tablename");
}

TEST(testLexerIntegerLiteral) {
    Lexer lexer("12345");
    Token token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::INT_LITERAL));
    ASSERT_EQ(token.value, "12345");
}

TEST(testLexerFloatLiteral) {
    Lexer lexer("3.14159");
    Token token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::FLOAT_LITERAL));
    ASSERT_EQ(token.value, "3.14159");
}

TEST(testLexerFloatWithExponent) {
    Lexer lexer("1.5e10");
    Token token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::FLOAT_LITERAL));
    ASSERT_EQ(token.value, "1.5e10");
}

TEST(testLexerNegativeNumber) {
    Lexer lexer("-123");
    Token token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::OPERATOR));
    ASSERT_EQ(token.value, "-");
    token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::INT_LITERAL));
    ASSERT_EQ(token.value, "123");
}

TEST(testLexerSingleQuotedString) {
    Lexer lexer("'hello world'");
    Token token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::STRING_LITERAL));
    ASSERT_EQ(token.value, "hello world");
}

TEST(testLexerDoubleQuotedString) {
    Lexer lexer("\"test string\"");
    Token token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::STRING_LITERAL));
    ASSERT_EQ(token.value, "test string");
}

TEST(testLexerStringWithEscape) {
    Lexer lexer("'it\\'s a test'");
    Token token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::STRING_LITERAL));
    ASSERT_EQ(token.value, "it's a test");
}

TEST(testLexerUnterminatedString) {
    Lexer lexer("'unterminated");
    bool threwException = false;
    try {
        lexer.nextToken();
    } catch (const minisql::MiniSQLException& e) {
        threwException = true;
    }
    ASSERT_TRUE(threwException);
}

TEST(testLexerKeyword) {
    Lexer lexer("SELECT");
    Token token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::KEYWORD));
    ASSERT_EQ(token.value, "SELECT");
}

TEST(testLexerKeywordCaseInsensitive) {
    Lexer lexer("select FROM where");
    Token token1 = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token1.type), static_cast<int>(TokenType::KEYWORD));
    ASSERT_EQ(token1.value, "SELECT");

    Token token2 = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token2.type), static_cast<int>(TokenType::KEYWORD));
    ASSERT_EQ(token2.value, "FROM");

    Token token3 = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token3.type), static_cast<int>(TokenType::KEYWORD));
    ASSERT_EQ(token3.value, "WHERE");
}

TEST(testLexerIdentifier) {
    Lexer lexer("users");
    Token token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::IDENTIFIER));
    ASSERT_EQ(token.value, "users");
}

TEST(testLexerIdentifierWithUnderscore) {
    Lexer lexer("user_name");
    Token token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::IDENTIFIER));
    ASSERT_EQ(token.value, "user_name");
}

TEST(testLexerOperator) {
    Lexer lexer("=");
    Token token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::OPERATOR));
    ASSERT_EQ(token.value, "=");
}

TEST(testLexerComplexOperator) {
    Lexer lexer(">=");
    Token token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::OPERATOR));
    ASSERT_EQ(token.value, ">=");
}

TEST(testLexerDelimiters) {
    Lexer lexer(",;()");
    Token token;

    token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::COMMA));

    token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::SEMICOLON));

    token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::LEFT_PAREN));

    token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::RIGHT_PAREN));
}

TEST(testLexerAllOperators) {
    Lexer lexer("= < > <= >= <> != + - * / %");
    Token token;

    token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::OPERATOR));
    ASSERT_EQ(token.value, "=");

    token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::OPERATOR));
    ASSERT_EQ(token.value, "<");

    token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::OPERATOR));
    ASSERT_EQ(token.value, ">");

    token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::OPERATOR));
    ASSERT_EQ(token.value, "<=");

    token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::OPERATOR));
    ASSERT_EQ(token.value, ">=");

    token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::OPERATOR));
    ASSERT_EQ(token.value, "<>");

    token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::OPERATOR));
    ASSERT_EQ(token.value, "!=");

    token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::OPERATOR));
    ASSERT_EQ(token.value, "+");

    token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::OPERATOR));
    ASSERT_EQ(token.value, "-");

    token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::OPERATOR));
    ASSERT_EQ(token.value, "*");

    token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::OPERATOR));
    ASSERT_EQ(token.value, "/");

    token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::OPERATOR));
    ASSERT_EQ(token.value, "%");
}

TEST(testLexerDotOperator) {
    Lexer lexer("mytable.column");
    Token token;

    token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::IDENTIFIER));
    ASSERT_EQ(token.value, "mytable");

    token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::DOT));
    ASSERT_EQ(token.value, ".");

    token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::IDENTIFIER));
    ASSERT_EQ(token.value, "column");
}

TEST(testLexerSingleLineComment) {
    Lexer lexer("-- this is a comment\nSELECT");
    Token token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::KEYWORD));
    ASSERT_EQ(token.value, "SELECT");
}

TEST(testLexerMultiLineComment) {
    Lexer lexer("/* this is\n   a comment */ SELECT");
    Token token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::KEYWORD));
    ASSERT_EQ(token.value, "SELECT");
}

TEST(testLexerCommentAtEnd) {
    Lexer lexer("SELECT -- comment");
    Token token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::KEYWORD));
    ASSERT_EQ(token.value, "SELECT");
    token = lexer.nextToken();
    ASSERT_EQ(static_cast<int>(token.type), static_cast<int>(TokenType::EOF_TOKEN));
}

TEST(testLexerSelectStatement) {
    Lexer lexer("SELECT id, name FROM users WHERE age > 18;");
    auto tokens = lexer.getAllTokens();

    ASSERT_GE(tokens.size(), 11);

    ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::KEYWORD));
    ASSERT_EQ(tokens[0].value, "SELECT");

    ASSERT_EQ(static_cast<int>(tokens[1].type), static_cast<int>(TokenType::IDENTIFIER));
    ASSERT_EQ(tokens[1].value, "id");

    ASSERT_EQ(static_cast<int>(tokens[2].type), static_cast<int>(TokenType::COMMA));

    ASSERT_EQ(static_cast<int>(tokens[3].type), static_cast<int>(TokenType::IDENTIFIER));
    ASSERT_EQ(tokens[3].value, "name");

    ASSERT_EQ(static_cast<int>(tokens[4].type), static_cast<int>(TokenType::KEYWORD));
    ASSERT_EQ(tokens[4].value, "FROM");

    ASSERT_EQ(static_cast<int>(tokens[5].type), static_cast<int>(TokenType::IDENTIFIER));
    ASSERT_EQ(tokens[5].value, "users");

    ASSERT_EQ(static_cast<int>(tokens[6].type), static_cast<int>(TokenType::KEYWORD));
    ASSERT_EQ(tokens[6].value, "WHERE");

    ASSERT_EQ(static_cast<int>(tokens[7].type), static_cast<int>(TokenType::IDENTIFIER));
    ASSERT_EQ(tokens[7].value, "age");

    ASSERT_EQ(static_cast<int>(tokens[8].type), static_cast<int>(TokenType::OPERATOR));
    ASSERT_EQ(tokens[8].value, ">");

    ASSERT_EQ(static_cast<int>(tokens[9].type), static_cast<int>(TokenType::INT_LITERAL));
    ASSERT_EQ(tokens[9].value, "18");

    ASSERT_EQ(static_cast<int>(tokens[10].type), static_cast<int>(TokenType::SEMICOLON));
}

TEST(testLexerInsertStatement) {
    Lexer lexer("INSERT INTO users (id, name) VALUES (1, 'Alice');");
    auto tokens = lexer.getAllTokens();

    ASSERT_EQ(static_cast<int>(tokens[0].type), static_cast<int>(TokenType::KEYWORD));
    ASSERT_EQ(tokens[0].value, "INSERT");

    ASSERT_EQ(static_cast<int>(tokens[1].type), static_cast<int>(TokenType::KEYWORD));
    ASSERT_EQ(tokens[1].value, "INTO");

    ASSERT_EQ(static_cast<int>(tokens[2].type), static_cast<int>(TokenType::IDENTIFIER));
    ASSERT_EQ(tokens[2].value, "users");
}

TEST(testLexerComplexSelect) {
    Lexer lexer("SELECT * FROM orders WHERE customer_id = 123 AND status = 'active' ORDER BY created_at DESC");
    auto tokens = lexer.getAllTokens();

    ASSERT_GE(tokens.size(), 15);

    ASSERT_EQ(tokens[0].value, "SELECT");
    ASSERT_EQ(tokens[2].value, "FROM");
    ASSERT_EQ(tokens[4].value, "WHERE");
    ASSERT_EQ(tokens[8].value, "AND");

    // 找到 ORDER 和 BY 的位置
    int orderIndex = -1, byIndex = -1;
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].value == "ORDER") orderIndex = i;
        if (tokens[i].value == "BY") byIndex = i;
    }
    ASSERT_GE(orderIndex, 0);
    ASSERT_GE(byIndex, 0);
}

// ============================================================
// 主函数
// ============================================================

int main() {
    std::cout << "=== Lexer Tests ===" << std::endl;

    testTokenTypeEnumExists();
    testTokenStructExists();
    testKeywordsIsKeyword();
    testKeywordsGetKeywordType();
    testLexerEmptyInput();
    testLexerSkipWhitespace();
    testLexerSingleIdentifier();
    testLexerIntegerLiteral();
    testLexerFloatLiteral();
    testLexerFloatWithExponent();
    testLexerNegativeNumber();
    testLexerSingleQuotedString();
    testLexerDoubleQuotedString();
    testLexerStringWithEscape();
    testLexerUnterminatedString();
    testLexerKeyword();
    testLexerKeywordCaseInsensitive();
    testLexerIdentifier();
    testLexerIdentifierWithUnderscore();
    testLexerOperator();
    testLexerComplexOperator();
    testLexerDelimiters();
    testLexerAllOperators();
    testLexerDotOperator();
    testLexerSingleLineComment();
    testLexerMultiLineComment();
    testLexerCommentAtEnd();
    testLexerSelectStatement();
    testLexerInsertStatement();
    testLexerComplexSelect();

    RUN_TESTS;

    return 0;
}
