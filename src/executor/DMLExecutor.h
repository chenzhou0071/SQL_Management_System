#pragma once

#include "../parser/AST.h"
#include "../common/Error.h"
#include "../common/Type.h"
#include "ExecutionOperator.h"
#include "ExpressionEvaluator.h"
#include "../storage/TableManager.h"
#include <string>
#include <vector>
#include <memory>

namespace minisql {
namespace executor {

// DML 执行器 - 处理 INSERT/UPDATE/DELETE 操作
class DMLExecutor {
public:
    // 执行 INSERT 语句
    static Result<ExecutionResult> executeInsert(const std::string& dbName, parser::InsertStmt* stmt);

    // 执行 UPDATE 语句
    static Result<ExecutionResult> executeUpdate(const std::string& dbName, parser::UpdateStmt* stmt);

    // 执行 DELETE 语句
    static Result<ExecutionResult> executeDelete(const std::string& dbName, parser::DeleteStmt* stmt);

    // 执行 SELECT 语句（使用算子树）
    static Result<ExecutionResult> executeSelect(const std::string& dbName, parser::SelectStmt* stmt);

private:
    // 评估行值（用于 INSERT）
    static Result<Tuple> evaluateRowValues(const std::vector<parser::ExprPtr>& exprs,
                                         ExpressionEvaluator& evaluator,
                                         const std::vector<std::string>& columnNames,
                                         const std::vector<DataType>& columnTypes);

    // 获取当前数据库名
    static std::string getCurrentDatabase();

    // 设置结果元数据
    static void setResultMetadata(ExecutionResult& result, const std::vector<std::string>& columnNames,
                                   const std::vector<DataType>& columnTypes);

    // 外键约束检查
    static Result<void> checkForeignKeyConstraints(
        const std::string& dbName,
        const std::string& tableName,
        const TableDef& tableDef,
        const Row& row,
        bool isInsert);

    static Result<void> checkForeignKeyForColumn(
        const std::string& dbName,
        const std::string& tableName,
        const ForeignKeyDef& fk,
        const Value& value);

    static bool hasReferencingRows(
        const std::string& dbName,
        const std::string& refTable,
        const std::string& refColumn,
        const Value& value);
};

} // namespace executor
} // namespace minisql
