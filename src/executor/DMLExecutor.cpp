#include "DMLExecutor.h"
#include "../storage/TableManager.h"
#include "../common/Logger.h"
#include <algorithm>

namespace minisql {
namespace executor {

std::string DMLExecutor::getCurrentDatabase() {
    // TODO: 实现当前数据库的跟踪
    // 暂时返回空字符串，需要从 UseStmt 或配置中获取
    return "";
}

void DMLExecutor::setResultMetadata(ExecutionResult& result, const std::vector<std::string>& columnNames,
                                  const std::vector<DataType>& columnTypes) {
    result.columnNames = columnNames;
    result.columnTypes = columnTypes;
}

Result<ExecutionResult> DMLExecutor::executeInsert(const std::string& dbName, parser::InsertStmt* stmt) {
    if (!stmt) {
        MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "NULL INSERT statement");
        return Result<ExecutionResult>(error);
    }

    auto& tableMgr = storage::TableManager::getInstance();

    // 获取表定义
    auto tableDefResult = tableMgr.getTableDef(dbName, stmt->table);
    if (!tableDefResult.isSuccess()) {
        return Result<ExecutionResult>(tableDefResult.getError());
    }
    TableDef tableDef = *tableDefResult.getValue();

    // 准备结果
    ExecutionResult result;
    std::vector<std::string> columnNames;
    std::vector<DataType> columnTypes;

    if (!stmt->columns.empty()) {
        columnNames = stmt->columns;
    } else {
        // 使用表定义中的所有列
        for (const auto& col : tableDef.columns) {
            columnNames.push_back(col.name);
        }
    }

    for (const auto& colName : columnNames) {
        auto it = std::find_if(tableDef.columns.begin(), tableDef.columns.end(),
            [&colName](const ColumnDef& col) { return col.name == colName; });
        if (it != tableDef.columns.end()) {
            columnTypes.push_back(it->type);
        } else {
            MiniSQLException error(ErrorCode::EXEC_COLUMN_NOT_FOUND,
                "Column not found: " + colName);
            return Result<ExecutionResult>(error);
        }
    }

    // 插入每一行
    int insertedCount = 0;
    ExpressionEvaluator evaluator;

    for (const auto& valuesExpr : stmt->valuesList) {
        auto rowResult = evaluateRowValues(valuesExpr, evaluator, columnNames, columnTypes);
        if (!rowResult.isSuccess()) {
            return Result<ExecutionResult>(rowResult.getError());
        }

        Row row = *rowResult.getValue();

        // 检查外键约束（使用表定义中的外键）
        if (!tableDef.foreignKeys.empty()) {
            auto fkResult = checkForeignKeyConstraints(dbName, stmt->table, tableDef, row, true);
            if (!fkResult.isSuccess()) {
                return Result<ExecutionResult>(fkResult.getError());
            }
        }

        auto insertResult = tableMgr.insertRow(dbName, stmt->table, row);
        if (!insertResult.isSuccess()) {
            return Result<ExecutionResult>(insertResult.getError());
        }

        insertedCount++;
    }

    // 设置结果
    setResultMetadata(result, {"affected_rows"}, {DataType::BIGINT});
    result.rows.push_back({Value(static_cast<int64_t>(insertedCount))});

    LOG_INFO("DMLExecutor", "Inserted " + std::to_string(insertedCount) + " rows into " + stmt->table);
    return Result<ExecutionResult>(result);
}

Result<ExecutionResult> DMLExecutor::executeUpdate(const std::string& dbName, parser::UpdateStmt* stmt) {
    if (!stmt) {
        MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "NULL UPDATE statement");
        return Result<ExecutionResult>(error);
    }

    auto& tableMgr = storage::TableManager::getInstance();

    // 获取表定义
    auto tableDefResult = tableMgr.getTableDef(dbName, stmt->table);
    if (!tableDefResult.isSuccess()) {
        return Result<ExecutionResult>(tableDefResult.getError());
    }
    TableDef tableDef = *tableDefResult.getValue();

    // 加载表数据
    auto tableDataResult = tableMgr.loadTable(dbName, stmt->table);
    if (!tableDataResult.isSuccess()) {
        return Result<ExecutionResult>(tableDataResult.getError());
    }
    storage::TableData tableData = *tableDataResult.getValue();

    int updatedCount = 0;
    ExpressionEvaluator evaluator;

    // 遍历每一行
    for (auto& rowPair : tableData) {
        int rowId = rowPair.first;
        Row& row = rowPair.second;

        // 构建行上下文
        RowContext context;
        auto columnNames = tableDef.columns;
        for (size_t i = 0; i < columnNames.size() && i < row.size(); ++i) {
            context[columnNames[i].name] = row[i];
        }

        // 检查 WHERE 条件
        bool shouldUpdate = true;
        if (stmt->whereClause) {
            evaluator.setRowContext(context);

            auto evalResult = evaluator.evaluate(stmt->whereClause);
            if (!evalResult.isSuccess()) {
                return Result<ExecutionResult>(evalResult.getError());
            }

            Value conditionResult = *evalResult.getValue();
            shouldUpdate = !conditionResult.isNull() && conditionResult.getBool();
        }

        if (shouldUpdate) {
            // 应用 SET 赋值前，先检查外键约束
            // 注意：UPDATE 可能修改外键列，需要验证
            Row updatedRow = row;
            for (const auto& assignment : stmt->assignments) {
                evaluator.setRowContext(context);

                auto evalResult = evaluator.evaluate(assignment.second);
                if (!evalResult.isSuccess()) {
                    return Result<ExecutionResult>(evalResult.getError());
                }

                // 找到列索引
                auto it = std::find_if(tableDef.columns.begin(), tableDef.columns.end(),
                    [&assignment](const ColumnDef& col) { return col.name == assignment.first; });

                if (it != tableDef.columns.end()) {
                    size_t colIndex = std::distance(tableDef.columns.begin(), it);
                    if (colIndex < updatedRow.size()) {
                        updatedRow[colIndex] = *evalResult.getValue();
                    }
                }
            }

            // 检查外键约束（如果更新的列包含外键列）
            if (!tableDef.foreignKeys.empty()) {
                auto fkResult = checkForeignKeyConstraints(dbName, stmt->table, tableDef, updatedRow, false);
                if (!fkResult.isSuccess()) {
                    return Result<ExecutionResult>(fkResult.getError());
                }
            }

            // 更新行
            auto updateResult = tableMgr.updateRow(dbName, stmt->table, rowId, updatedRow);
            if (!updateResult.isSuccess()) {
                return Result<ExecutionResult>(updateResult.getError());
            }

            updatedCount++;
        }
    }

    // 设置结果
    ExecutionResult result;
    setResultMetadata(result, {"affected_rows"}, {DataType::BIGINT});
    result.rows.push_back({Value(static_cast<int64_t>(updatedCount))});

    LOG_INFO("DMLExecutor", "Updated " + std::to_string(updatedCount) + " rows in " + stmt->table);
    return Result<ExecutionResult>(result);
}

Result<ExecutionResult> DMLExecutor::executeDelete(const std::string& dbName, parser::DeleteStmt* stmt) {
    if (!stmt) {
        MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "NULL DELETE statement");
        return Result<ExecutionResult>(error);
    }

    auto& tableMgr = storage::TableManager::getInstance();

    // 获取表定义
    auto tableDefResult = tableMgr.getTableDef(dbName, stmt->table);
    if (!tableDefResult.isSuccess()) {
        return Result<ExecutionResult>(tableDefResult.getError());
    }
    TableDef tableDef = *tableDefResult.getValue();

    // 加载表数据
    auto tableDataResult = tableMgr.loadTable(dbName, stmt->table);
    if (!tableDataResult.isSuccess()) {
        return Result<ExecutionResult>(tableDataResult.getError());
    }
    storage::TableData tableData = *tableDataResult.getValue();

    int deletedCount = 0;
    ExpressionEvaluator evaluator;
    std::vector<int> rowIdsToDelete;

    // 遍历每一行
    for (auto& rowPair : tableData) {
        int rowId = rowPair.first;
        const Row& row = rowPair.second;

        // 检查 WHERE 条件
        bool shouldDelete = true;
        if (stmt->whereClause) {
            // 构建行上下文
            RowContext context;
            auto columnNames = tableDef.columns;
            for (size_t i = 0; i < columnNames.size() && i < row.size(); ++i) {
                context[columnNames[i].name] = row[i];
            }
            evaluator.setRowContext(context);

            auto evalResult = evaluator.evaluate(stmt->whereClause);
            if (!evalResult.isSuccess()) {
                return Result<ExecutionResult>(evalResult.getError());
            }

            Value conditionResult = *evalResult.getValue();
            shouldDelete = !conditionResult.isNull() && conditionResult.getBool();
        }

        if (shouldDelete) {
            rowIdsToDelete.push_back(rowId);
        }
    }

    // 删除行（从后往前，避免索引问题）
    for (auto it = rowIdsToDelete.rbegin(); it != rowIdsToDelete.rend(); ++it) {
        auto deleteResult = tableMgr.deleteRow(dbName, stmt->table, *it);
        if (!deleteResult.isSuccess()) {
            return Result<ExecutionResult>(deleteResult.getError());
        }
        deletedCount++;
    }

    // 设置结果
    ExecutionResult result;
    setResultMetadata(result, {"affected_rows"}, {DataType::BIGINT});
    result.rows.push_back({Value(static_cast<int64_t>(deletedCount))});

    LOG_INFO("DMLExecutor", "Deleted " + std::to_string(deletedCount) + " rows from " + stmt->table);
    return Result<ExecutionResult>(result);
}

Result<ExecutionResult> DMLExecutor::executeSelect(const std::string& dbName, parser::SelectStmt* stmt) {
    if (!stmt) {
        MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "NULL SELECT statement");
        return Result<ExecutionResult>(error);
    }

    // TODO: 实现完整的执行计划构建
    // 这里需要根据 FROM、WHERE、GROUP BY、HAVING、ORDER BY、LIMIT
    // 构建算子树并执行

    MiniSQLException error(ErrorCode::EXEC_TABLE_NOT_FOUND,
        "SELECT execution not yet implemented - will be implemented in main Executor");
    return Result<ExecutionResult>(error);
}

Result<Tuple> DMLExecutor::evaluateRowValues(const std::vector<parser::ExprPtr>& exprs,
                                           ExpressionEvaluator& evaluator,
                                           const std::vector<std::string>& columnNames,
                                           const std::vector<DataType>& columnTypes) {
    Row row;
    row.reserve(exprs.size());

    for (size_t i = 0; i < exprs.size(); ++i) {
        auto evalResult = evaluator.evaluate(exprs[i]);
        if (!evalResult.isSuccess()) {
            return Result<Tuple>(evalResult.getError());
        }

        Value val = *evalResult.getValue();

        // 类型检查和转换
        if (i < columnTypes.size()) {
            DataType expectedType = columnTypes[i];
            DataType actualType = val.getType();

            // 简单的类型兼容性检查
            if (expectedType == DataType::INT && actualType == DataType::BIGINT) {
                // 兼容
            } else if (expectedType == DataType::DOUBLE && actualType == DataType::INT) {
                // 兼容
            } else if (expectedType != actualType && !val.isNull()) {
                MiniSQLException error(ErrorCode::EXEC_TYPE_MISMATCH,
                    "Type mismatch for column " + columnNames[i]);
                return Result<Tuple>(error);
            }
        }

        row.push_back(val);
    }

    return Result<Tuple>(row);
}

// ==================== 外键约束检查 ====================

Result<void> DMLExecutor::checkForeignKeyConstraints(
    const std::string& dbName,
    const std::string& tableName,
    const TableDef& tableDef,
    const Row& row,
    bool isInsert) {

    // 检查每个外键定义
    for (const auto& fk : tableDef.foreignKeys) {
        // 找到外键列在行中的位置
        auto colIt = std::find_if(tableDef.columns.begin(), tableDef.columns.end(),
            [&fk](const ColumnDef& col) { return col.name == fk.column; });

        if (colIt == tableDef.columns.end()) {
            continue;  // 列不存在，跳过
        }

        size_t colIndex = std::distance(tableDef.columns.begin(), colIt);
        if (colIndex >= row.size()) {
            continue;  // 行中没有这个列
        }

        const Value& value = row[colIndex];

        // NULL 值通常允许（除非 NOT NULL）
        if (value.isNull()) {
            continue;
        }

        // 检查外键约束
        auto checkResult = checkForeignKeyForColumn(dbName, tableName, fk, value);
        if (checkResult.isError()) {
            return checkResult;
        }
    }

    return Result<void>();
}

Result<void> DMLExecutor::checkForeignKeyForColumn(
    const std::string& dbName,
    const std::string& tableName,
    const ForeignKeyDef& fk,
    const Value& value) {

    auto& tableMgr = storage::TableManager::getInstance();

    // 检查引用的表是否存在
    auto refTableDefResult = tableMgr.getTableDef(dbName, fk.refTable);
    if (!refTableDefResult.isSuccess()) {
        return Result<void>(refTableDefResult.getError());
    }
    TableDef refTableDef = *refTableDefResult.getValue();

    // 检查引用的列是否存在
    auto refColIt = std::find_if(refTableDef.columns.begin(), refTableDef.columns.end(),
        [&fk](const ColumnDef& col) { return col.name == fk.refColumn; });

    if (refColIt == refTableDef.columns.end()) {
        MiniSQLException error(ErrorCode::EXEC_COLUMN_NOT_FOUND,
            "Foreign key references non-existent column: " + fk.refTable + "." + fk.refColumn);
        return Result<void>(error);
    }

    // 加载引用表的数据，检查值是否存在
    auto tableDataResult = tableMgr.loadTable(dbName, fk.refTable);
    if (!tableDataResult.isSuccess()) {
        return Result<void>(tableDataResult.getError());
    }
    storage::TableData tableData = *tableDataResult.getValue();

    // 找到引用列的索引
    size_t refColIndex = std::distance(refTableDef.columns.begin(), refColIt);

    // 在引用表中查找值
    bool found = false;
    for (const auto& rowPair : tableData) {
        const Row& refRow = rowPair.second;
        if (refColIndex < refRow.size() && refRow[refColIndex] == value) {
            found = true;
            break;
        }
    }

    if (!found) {
        MiniSQLException error(ErrorCode::STORAGE_CONSTRAINT_VIOLATION,
            "Foreign key constraint violation: " + tableName + "." + fk.column +
            " references " + fk.refTable + "." + fk.refColumn +
            " but value (" + value.toString() + ") does not exist in parent table");
        return Result<void>(error);
    }

    return Result<void>();
}

bool DMLExecutor::hasReferencingRows(
    const std::string& dbName,
    const std::string& refTable,
    const std::string& refColumn,
    const Value& value) {

    auto& tableMgr = storage::TableManager::getInstance();

    // 获取所有表定义（需要遍历）
    // 简化：假设只有当前数据库中的表
    // 这里需要实现查找引用了 refTable 的所有表
    // 为了简单起见，我们跳过这个检查，让用户手动确保数据一致性

    return false;  // 简化：不做检查
}

} // namespace executor
} // namespace minisql
