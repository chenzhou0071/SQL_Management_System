#include "DDLExecutor.h"
#include "../storage/TableManager.h"
#include "../storage/IndexManager.h"
#include "../common/Logger.h"
#include <algorithm>

namespace minisql {
namespace executor {

void DDLExecutor::setResultMetadata(ExecutionResult& result, const std::vector<std::string>& columnNames,
                                   const std::vector<DataType>& columnTypes) {
    result.columnNames = columnNames;
    result.columnTypes = columnTypes;
}

ColumnDef DDLExecutor::convertColumnDef(const parser::ColumnDefNode& node) {
    ColumnDef col;
    col.name = node.name;
    col.type = node.type;
    col.length = node.length;
    col.notNull = node.notNull;
    col.primaryKey = node.primaryKey;
    col.unique = node.unique;
    col.autoIncrement = node.autoIncrement;
    col.defaultValue = node.defaultValue;
    return col;
}

Result<ExecutionResult> DDLExecutor::executeCreateDatabase(parser::CreateDatabaseStmt* stmt) {
    if (!stmt) {
        MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "NULL CREATE DATABASE statement");
        return Result<ExecutionResult>(error);
    }

    if (stmt->database.empty()) {
        MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "Database name cannot be empty");
        return Result<ExecutionResult>(error);
    }

    auto& tableMgr = storage::TableManager::getInstance();
    auto result = tableMgr.createDatabase(stmt->database);

    ExecutionResult execResult;
    if (result.isSuccess()) {
        setResultMetadata(execResult, {"status"}, {DataType::VARCHAR});
        execResult.rows.push_back({Value("Database created successfully")});
        LOG_INFO("DDLExecutor", "Created database: " + stmt->database);
        return Result<ExecutionResult>(execResult);
    } else {
        return Result<ExecutionResult>(result.getError());
    }
}

Result<ExecutionResult> DDLExecutor::executeCreateTable(const std::string& dbName, parser::CreateTableStmt* stmt) {
    if (!stmt) {
        MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "NULL CREATE TABLE statement");
        return Result<ExecutionResult>(error);
    }

    auto& tableMgr = storage::TableManager::getInstance();

    // 转换表定义
    TableDef tableDef;
    tableDef.name = stmt->table;
    tableDef.database = dbName;

    for (const auto& colNode : stmt->columns) {
        tableDef.columns.push_back(convertColumnDef(*colNode));
    }

    // 复制外键定义
    for (const auto& fk : stmt->foreignKeys) {
        tableDef.foreignKeys.push_back(fk);
    }

    // 复制索引定义
    for (const auto& idx : stmt->indexes) {
        tableDef.indexes.push_back(idx);
    }

    auto result = tableMgr.createTable(dbName, tableDef);

    ExecutionResult execResult;
    if (result.isSuccess()) {
        setResultMetadata(execResult, {"status"}, {DataType::VARCHAR});
        execResult.rows.push_back({Value("Table created successfully")});
        LOG_INFO("DDLExecutor", "Created table: " + stmt->table);
        return Result<ExecutionResult>(execResult);
    } else {
        return Result<ExecutionResult>(result.getError());
    }
}

Result<ExecutionResult> DDLExecutor::executeDrop(const std::string& dbName, parser::DropStmt* stmt) {
    if (!stmt) {
        MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "NULL DROP statement");
        return Result<ExecutionResult>(error);
    }

    auto& tableMgr = storage::TableManager::getInstance();
    ExecutionResult execResult;

    std::string objectTypeUpper = stmt->objectType;
    std::transform(objectTypeUpper.begin(), objectTypeUpper.end(), objectTypeUpper.begin(),
        [](unsigned char c) { return std::toupper(c); });

    if (objectTypeUpper == "TABLE") {
        auto result = tableMgr.dropTable(dbName, stmt->name);
        if (result.isSuccess()) {
            setResultMetadata(execResult, {"status"}, {DataType::VARCHAR});
            execResult.rows.push_back({Value("Table dropped successfully")});
            LOG_INFO("DDLExecutor", "Dropped table: " + stmt->name);
            return Result<ExecutionResult>(execResult);
        } else {
            return Result<ExecutionResult>(result.getError());
        }
    } else if (objectTypeUpper == "DATABASE") {
        auto result = tableMgr.dropDatabase(stmt->name);
        if (result.isSuccess()) {
            setResultMetadata(execResult, {"status"}, {DataType::VARCHAR});
            execResult.rows.push_back({Value("Database dropped successfully")});
            LOG_INFO("DDLExecutor", "Dropped database: " + stmt->name);
            return Result<ExecutionResult>(execResult);
        } else {
            return Result<ExecutionResult>(result.getError());
        }
    } else if (objectTypeUpper == "INDEX") {
        auto& indexMgr = storage::IndexManager::getInstance();
        auto result = indexMgr.dropIndex(dbName, stmt->name);
        if (result.isSuccess()) {
            setResultMetadata(execResult, {"status"}, {DataType::VARCHAR});
            execResult.rows.push_back({Value("Index dropped successfully")});
            LOG_INFO("DDLExecutor", "Dropped index: " + stmt->name);
            return Result<ExecutionResult>(execResult);
        } else {
            return Result<ExecutionResult>(result.getError());
        }
    } else {
        MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE,
            "Unsupported DROP object type: " + stmt->objectType);
        return Result<ExecutionResult>(error);
    }
}

Result<ExecutionResult> DDLExecutor::executeAlterTable(const std::string& dbName, parser::AlterTableStmt* stmt) {
    if (!stmt) {
        MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "NULL ALTER TABLE statement");
        return Result<ExecutionResult>(error);
    }

    auto& tableMgr = storage::TableManager::getInstance();
    ExecutionResult execResult;

    switch (stmt->operation) {
        case parser::AlterOperationType::ADD_COLUMN: {
            if (!stmt->columnDef) {
                MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "Missing column definition for ADD COLUMN");
                return Result<ExecutionResult>(error);
            }
            ColumnDef col = convertColumnDef(*stmt->columnDef);
            auto result = tableMgr.addColumn(dbName, stmt->tableName, col);
            if (result.isSuccess()) {
                setResultMetadata(execResult, {"status"}, {DataType::VARCHAR});
                execResult.rows.push_back({Value("Column added successfully")});
                LOG_INFO("DDLExecutor", "Added column " + col.name + " to table " + stmt->tableName);
                return Result<ExecutionResult>(execResult);
            } else {
                return Result<ExecutionResult>(result.getError());
            }
        }

        case parser::AlterOperationType::DROP_COLUMN: {
            if (!stmt->columnDef) {
                MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "Missing column name for DROP COLUMN");
                return Result<ExecutionResult>(error);
            }
            auto result = tableMgr.dropColumn(dbName, stmt->tableName, stmt->columnDef->name);
            if (result.isSuccess()) {
                setResultMetadata(execResult, {"status"}, {DataType::VARCHAR});
                execResult.rows.push_back({Value("Column dropped successfully")});
                LOG_INFO("DDLExecutor", "Dropped column " + stmt->columnDef->name + " from table " + stmt->tableName);
                return Result<ExecutionResult>(execResult);
            } else {
                return Result<ExecutionResult>(result.getError());
            }
        }

        case parser::AlterOperationType::RENAME_TABLE: {
            if (stmt->newTableName.empty()) {
                MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "Missing new table name for RENAME TABLE");
                return Result<ExecutionResult>(error);
            }
            auto result = tableMgr.renameTable(dbName, stmt->tableName, stmt->newTableName);
            if (result.isSuccess()) {
                setResultMetadata(execResult, {"status"}, {DataType::VARCHAR});
                execResult.rows.push_back({Value("Table renamed successfully")});
                LOG_INFO("DDLExecutor", "Renamed table " + stmt->tableName + " to " + stmt->newTableName);
                return Result<ExecutionResult>(execResult);
            } else {
                return Result<ExecutionResult>(result.getError());
            }
        }

        default: {
            MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE,
                "Unsupported ALTER TABLE operation");
            return Result<ExecutionResult>(error);
        }
    }
}

Result<ExecutionResult> DDLExecutor::executeUseDatabase(parser::UseStmt* stmt) {
    if (!stmt) {
        MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "NULL USE statement");
        return Result<ExecutionResult>(error);
    }

    auto& tableMgr = storage::TableManager::getInstance();

    // 检查数据库是否存在
    if (!tableMgr.databaseExists(stmt->database)) {
        MiniSQLException error(ErrorCode::STORAGE_TABLE_NOT_FOUND,
            "Database not found: " + stmt->database);
        return Result<ExecutionResult>(error);
    }

    ExecutionResult execResult;
    setResultMetadata(execResult, {"database"}, {DataType::VARCHAR});
    execResult.rows.push_back({Value(stmt->database)});
    LOG_INFO("DDLExecutor", "Using database: " + stmt->database);
    return Result<ExecutionResult>(execResult);
}

Result<ExecutionResult> DDLExecutor::executeCreateIndex(const std::string& dbName, parser::CreateIndexStmt* stmt) {
    if (!stmt) {
        MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "NULL CREATE INDEX statement");
        return Result<ExecutionResult>(error);
    }

    if (stmt->indexName.empty() || stmt->tableName.empty() || stmt->columnNames.empty()) {
        MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "Index name, table name, and column names are required");
        return Result<ExecutionResult>(error);
    }

    auto& indexMgr = storage::IndexManager::getInstance();

    // 目前只支持单列索引，使用第一列
    std::string columnName = stmt->columnNames[0];
    IndexUsageType type = stmt->unique ? IndexUsageType::UNIQUE : IndexUsageType::NORMAL;

    auto result = indexMgr.createIndex(dbName, stmt->tableName, stmt->indexName, columnName, type);

    ExecutionResult execResult;
    if (result.isSuccess()) {
        setResultMetadata(execResult, {"status"}, {DataType::VARCHAR});
        execResult.rows.push_back({Value("Index created successfully")});
        LOG_INFO("DDLExecutor", "Created index: " + stmt->indexName + " on " + stmt->tableName);
        return Result<ExecutionResult>(execResult);
    } else {
        return Result<ExecutionResult>(result.getError());
    }
}

} // namespace executor
} // namespace minisql
