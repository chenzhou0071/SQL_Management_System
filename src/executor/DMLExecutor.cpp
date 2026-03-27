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

            const Value& conditionResult = *evalResult.getValue();
            shouldUpdate = !conditionResult.isNull() && conditionResult.getBool();
        }

        if (shouldUpdate) {
            // 应用 SET 赋值
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
                    if (colIndex < row.size()) {
                        row[colIndex] = *evalResult.getValue();
                    }
                }
            }

            // 更新行
            auto updateResult = tableMgr.updateRow(dbName, stmt->table, rowId, row);
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

            const Value& conditionResult = *evalResult.getValue();
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

} // namespace executor
} // namespace minisql
