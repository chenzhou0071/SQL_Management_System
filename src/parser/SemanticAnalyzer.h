#pragma once

#include "AST.h"
#include "../storage/TableManager.h"
#include <string>
#include <memory>

namespace minisql {
namespace parser {

// ============================================================
// 语义错误
// ============================================================
class SemanticError : public std::exception {
public:
    SemanticError(const std::string& message) : message_(message) {}
    const char* what() const noexcept override { return message_.c_str(); }

private:
    std::string message_;
};

// ============================================================
// 语义分析器
// ============================================================
class SemanticAnalyzer {
public:
    SemanticAnalyzer();

    // 分析语句
    void analyze(std::shared_ptr<ASTNode> stmt);

    // 分析 SELECT 语句
    void analyzeSelect(std::shared_ptr<SelectStmt> stmt);

    // 分析 INSERT 语句
    void analyzeInsert(std::shared_ptr<InsertStmt> stmt);

    // 分析 UPDATE 语句
    void analyzeUpdate(std::shared_ptr<UpdateStmt> stmt);

    // 分析 DELETE 语句
    void analyzeDelete(std::shared_ptr<DeleteStmt> stmt);

    // 分析 CREATE TABLE 语句
    void analyzeCreateTable(std::shared_ptr<CreateTableStmt> stmt);

    // 分析 DROP 语句
    void analyzeDrop(std::shared_ptr<DropStmt> stmt);

    // 分析 USE 语句
    void analyzeUse(std::shared_ptr<UseStmt> stmt);

    // 分析 SHOW 语句
    void analyzeShow(std::shared_ptr<ShowStmt> stmt);

private:
    // 获取当前数据库
    std::string getCurrentDatabase();

    // 检查表是否存在
    void checkTableExists(const std::string& table);

    // 检查列是否存在
    void checkColumnExists(const std::string& table, const std::string& column);

    // 检查数据类型
    void checkDataType(DataType actual, DataType expected);

    // 验证表达式
    void validateExpression(ExprPtr expr, const std::string& contextTable);

    storage::TableManager& tableManager_;
};

}  // namespace parser
}  // namespace minisql
