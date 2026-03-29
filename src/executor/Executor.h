#pragma once

#include "../parser/AST.h"
#include "../common/Error.h"
#include "ExecutionOperator.h"
#include <string>
#include <memory>

namespace minisql {
namespace executor {

// 主执行器 - 整合所有执行组件
class Executor {
public:
    // 获取单例实例
    static Executor& getInstance();

    // 执行 SQL 语句（使用单例的当前数据库）
    Result<ExecutionResult> execute(parser::ASTNode* stmt);

    // 执行 SQL 语句（使用指定的数据库名称）
    Result<ExecutionResult> execute(parser::ASTNode* stmt, const std::string& dbName);

    // 获取当前数据库
    std::string getCurrentDatabase() const { return currentDatabase_; }

    // 设置当前数据库
    void setCurrentDatabase(const std::string& dbName) { currentDatabase_ = dbName; }

private:
    Executor() = default;
    ~Executor() = default;
    Executor(const Executor&) = delete;
    Executor& operator=(const Executor&) = delete;

    // 从 SELECT 语句构建执行计划
    OperatorPtr buildExecutionPlan(parser::SelectStmt* stmt);

    // 执行 SELECT（使用算子树）
    Result<ExecutionResult> executeSelect(parser::SelectStmt* stmt);

    // 执行 SELECT（使用指定的数据库名称）
    Result<ExecutionResult> executeSelect(parser::SelectStmt* stmt, const std::string& dbName);

    std::string currentDatabase_;
};

} // namespace executor
} // namespace minisql
