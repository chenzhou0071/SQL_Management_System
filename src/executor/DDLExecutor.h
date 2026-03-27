#pragma once

#include "../parser/AST.h"
#include "../common/Error.h"
#include "ExecutionOperator.h"
#include "../storage/TableManager.h"
#include <string>

namespace minisql {
namespace executor {

// DDL 执行器 - 处理 CREATE/DROP/ALTER 操作
class DDLExecutor {
public:
    // 执行 CREATE DATABASE 语句
    static Result<ExecutionResult> executeCreateDatabase(const std::string& dbName);

    // 执行 CREATE TABLE 语句
    static Result<ExecutionResult> executeCreateTable(const std::string& dbName, parser::CreateTableStmt* stmt);

    // 执行 DROP 语句 (TABLE, DATABASE, INDEX, VIEW)
    static Result<ExecutionResult> executeDrop(const std::string& dbName, parser::DropStmt* stmt);

    // 执行 ALTER TABLE 语句
    static Result<ExecutionResult> executeAlterTable(const std::string& dbName, parser::AlterTableStmt* stmt);

    // 执行 USE DATABASE 语句
    static Result<ExecutionResult> executeUseDatabase(parser::UseStmt* stmt);

private:
    // 将 ColumnDefNode 转换为 ColumnDef
    static ColumnDef convertColumnDef(const parser::ColumnDefNode& node);

    // 设置结果元数据
    static void setResultMetadata(ExecutionResult& result, const std::vector<std::string>& columnNames,
                                   const std::vector<DataType>& columnTypes);
};

} // namespace executor
} // namespace minisql
