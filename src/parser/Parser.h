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
    std::shared_ptr<ASTNode> parseExplainStatement();
    std::shared_ptr<ASTNode> parseAnalyzeStatement();
    std::shared_ptr<SelectStmt> parseSelectStatement();
    std::shared_ptr<InsertStmt> parseInsertStatement();
    std::shared_ptr<UpdateStmt> parseUpdateStatement();
    std::shared_ptr<DeleteStmt> parseDeleteStatement();
    std::shared_ptr<ASTNode> parseCreateStatement();
    std::shared_ptr<DropStmt> parseDropStatement();
    std::shared_ptr<AlterTableStmt> parseAlterStatement();

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
