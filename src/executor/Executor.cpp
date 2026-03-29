#include "Executor.h"
#include "TableScanOperator.h"
#include "FilterOperator.h"
#include "ProjectOperator.h"
#include "NestedLoopJoinOperator.h"
#include "SortOperator.h"
#include "LimitOperator.h"
#include "AggregateOperator.h"
#include "SortOperator.h"
#include "DMLExecutor.h"
#include "DDLExecutor.h"
#include "../optimizer/QueryOptimizer.h"
#include "../optimizer/ExplainHandler.h"
#include "../optimizer/PlanGenerator.h"
#include "../optimizer/Statistics.h"
#include "../common/Logger.h"
#include <stdexcept>
#include <algorithm>

namespace minisql {
namespace executor {

// 从 selectItems 中提取聚合函数
static void extractAggregates(const std::vector<parser::ExprPtr>& selectItems,
                               std::vector<AggregateFunc>& aggregates,
                               std::vector<parser::ExprPtr>& nonAggregateItems) {
    for (const auto& item : selectItems) {
        // 检查是否是函数调用表达式
        if (item->getType() == parser::ASTNodeType::FUNCTION_CALL) {
            auto funcExpr = std::dynamic_pointer_cast<parser::FunctionCallExpr>(item);
            std::string funcName = funcExpr->getFuncName();

            // 检查是否是聚合函数
            std::string upperName = funcName;
            std::transform(upperName.begin(), upperName.end(), upperName.begin(),
                [](unsigned char c) { return std::toupper(c); });

            if (upperName == "COUNT" || upperName == "SUM" || upperName == "AVG" ||
                upperName == "MIN" || upperName == "MAX") {
                // 提取聚合函数
                auto args = funcExpr->getArgs();
                parser::ExprPtr arg = (args.empty()) ? nullptr : args[0];
                aggregates.emplace_back(upperName, arg);
                // 用标识符替代原聚合表达式（用于投影）
                nonAggregateItems.push_back(
                    std::make_shared<parser::ColumnRef>("", "_agg_" + std::to_string(aggregates.size() - 1)));
            } else {
                nonAggregateItems.push_back(item);
            }
        } else {
            nonAggregateItems.push_back(item);
        }
    }
}

Executor& Executor::getInstance() {
    static Executor instance;
    return instance;
}

Result<ExecutionResult> Executor::execute(parser::ASTNode* stmt) {
    if (!stmt) {
        MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "NULL statement");
        return Result<ExecutionResult>(error);
    }

    try {
        parser::ASTNodeType nodeType = stmt->getType();

        switch (nodeType) {
            case parser::ASTNodeType::INSERT_STMT: {
                auto insertStmt = dynamic_cast<parser::InsertStmt*>(stmt);
                return DMLExecutor::executeInsert(currentDatabase_, insertStmt);
            }

            case parser::ASTNodeType::UPDATE_STMT: {
                auto updateStmt = dynamic_cast<parser::UpdateStmt*>(stmt);
                return DMLExecutor::executeUpdate(currentDatabase_, updateStmt);
            }

            case parser::ASTNodeType::DELETE_STMT: {
                auto deleteStmt = dynamic_cast<parser::DeleteStmt*>(stmt);
                return DMLExecutor::executeDelete(currentDatabase_, deleteStmt);
            }

            case parser::ASTNodeType::SELECT_STMT: {
                // 绕过 dynamic_cast，直接使用 stmt
                return executeSelect(static_cast<parser::SelectStmt*>(stmt));
            }

            case parser::ASTNodeType::EXPLAIN_STMT: {
                auto explainStmt = dynamic_cast<parser::ExplainStmt*>(stmt);
                auto selectStmt = std::dynamic_pointer_cast<parser::SelectStmt>(explainStmt->innerStatement);
                if (!selectStmt) {
                    return Result<ExecutionResult>(
                        MiniSQLException(ErrorCode::EXEC_INVALID_VALUE, "EXPLAIN requires a SELECT statement"));
                }

                // 生成 EXPLAIN 输出
                auto explainResult = optimizer::ExplainHandler::explain(currentDatabase_, selectStmt.get());
                if (explainResult.isError()) {
                    return Result<ExecutionResult>(explainResult.getError());
                }

                ExecutionResult result;
                result.rows.push_back({});

                // 根据格式输出
                if (explainStmt->formatJSON) {
                    result.rows[0].push_back(
                        Value(optimizer::ExplainHandler::formatJSON(*explainResult.getValue())));
                } else {
                    result.rows[0].push_back(
                        Value(optimizer::ExplainHandler::formatTable(*explainResult.getValue())));
                }
                return Result<ExecutionResult>(result);
            }

            case parser::ASTNodeType::ANALYZE_STMT: {
                auto analyzeStmt = dynamic_cast<parser::AnalyzeStmt*>(stmt);

                // 调用统计信息收集
                auto analyzeResult = optimizer::Statistics::getInstance().analyzeTable(
                    currentDatabase_, analyzeStmt->tableName);

                ExecutionResult result;
                if (analyzeResult.isError()) {
                    return Result<ExecutionResult>(analyzeResult.getError());
                }

                // 返回成功信息
                result.rows.push_back({
                    Value("Table"),
                    Value("Op"),
                    Value("Msg_type"),
                    Value("Msg_text")
                });
                result.rows.push_back({
                    Value(analyzeStmt->tableName),
                    Value("analyze"),
                    Value("status"),
                    Value("OK")
                });
                return Result<ExecutionResult>(result);
            }

            case parser::ASTNodeType::CREATE_DATABASE_STMT: {
                auto createDBStmt = dynamic_cast<parser::CreateDatabaseStmt*>(stmt);
                return DDLExecutor::executeCreateDatabase(createDBStmt);
            }

            case parser::ASTNodeType::CREATE_INDEX_STMT: {
                auto createIdxStmt = dynamic_cast<parser::CreateIndexStmt*>(stmt);
                return DDLExecutor::executeCreateIndex(currentDatabase_, createIdxStmt);
            }

            case parser::ASTNodeType::CREATE_STMT: {
                auto createTableStmt = dynamic_cast<parser::CreateTableStmt*>(stmt);
                return DDLExecutor::executeCreateTable(currentDatabase_, createTableStmt);
            }

            case parser::ASTNodeType::DROP_STMT: {
                auto dropStmt = dynamic_cast<parser::DropStmt*>(stmt);
                return DDLExecutor::executeDrop(currentDatabase_, dropStmt);
            }

            case parser::ASTNodeType::ALTER_STMT: {
                auto alterStmt = dynamic_cast<parser::AlterTableStmt*>(stmt);
                return DDLExecutor::executeAlterTable(currentDatabase_, alterStmt);
            }

            case parser::ASTNodeType::USE_STMT: {
                auto useStmt = dynamic_cast<parser::UseStmt*>(stmt);
                auto result = DDLExecutor::executeUseDatabase(useStmt);
                if (result.isSuccess()) {
                    // 更新当前数据库
                    currentDatabase_ = useStmt->database;
                }
                return result;
            }

            default:
                return Result<ExecutionResult>(
                    MiniSQLException(ErrorCode::EXEC_INVALID_VALUE, "Unsupported statement type"));
        }
    } catch (const std::exception& e) {
        return Result<ExecutionResult>(
            MiniSQLException(ErrorCode::EXEC_ERROR_BASE, std::string("Execution error: ") + e.what()));
    }
}

Result<ExecutionResult> Executor::executeSelect(parser::SelectStmt* stmt) {
    if (!stmt) {
        MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "NULL SELECT statement");
        return Result<ExecutionResult>(error);
    }

    // ========== 使用 QueryOptimizer 进行完整优化流程 ==========
    LOG_INFO("Executor", "Starting optimized query execution");

    auto optimizeResult = optimizer::QueryOptimizer::optimize(currentDatabase_, stmt);
    if (optimizeResult.isError()) {
        return Result<ExecutionResult>(optimizeResult.getError());
    }

    OperatorPtr root = *optimizeResult.getValue();
    if (!root) {
        return Result<ExecutionResult>(
            MiniSQLException(ErrorCode::EXEC_ERROR_BASE, "Failed to build execution plan"));
    }

    // 执行算子树
    auto openResult = root->open();
    if (!openResult.isSuccess()) {
        return Result<ExecutionResult>(openResult.getError());
    }

    ExecutionResult result;
    result.columnNames = root->getColumnNames();
    result.columnTypes = root->getColumnTypes();

    // 获取所有行
    while (true) {
        auto nextResult = root->next();
        if (!nextResult.isSuccess()) {
            root->close();
            return Result<ExecutionResult>(nextResult.getError());
        }

        auto tuple = *nextResult.getValue();
        if (!tuple) {
            // 没有更多行
            break;
        }

        result.rows.push_back(*tuple);
    }

    root->close();

    LOG_INFO("Executor", "SELECT returned " + std::to_string(result.rows.size()) + " rows");
    return Result<ExecutionResult>(result);
}

OperatorPtr Executor::buildExecutionPlan(parser::SelectStmt* stmt) {
    // 获取表名
    std::string tableName;
    if (stmt->fromTable) {
        tableName = stmt->fromTable->name;
    } else {
        return nullptr;  // 没有 FROM 子句
    }

    // 提取聚合函数和非聚合项
    std::vector<AggregateFunc> aggregates;
    std::vector<parser::ExprPtr> nonAggregateItems;
    extractAggregates(stmt->selectItems, aggregates, nonAggregateItems);

    // 构建算子树（从底向上）
    // 基础：TableScan
    OperatorPtr current = std::make_shared<TableScanOperator>(currentDatabase_, tableName);

    // WHERE 过滤
    if (stmt->whereClause) {
        current = std::make_shared<FilterOperator>(current, stmt->whereClause);
    }

    // GROUP BY + 聚合
    if (!stmt->groupBy.empty() || !aggregates.empty()) {
        current = std::make_shared<AggregateOperator>(current, stmt->groupBy, aggregates);
    }

    // ORDER BY 排序
    if (!stmt->orderBy.empty()) {
        std::vector<SortItem> sortItems;
        for (const auto& item : stmt->orderBy) {
            sortItems.emplace_back(item.expr, item.ascending);
        }
        current = std::make_shared<SortOperator>(current, sortItems);
    }

    // SELECT 投影
    if (!stmt->selectItems.empty()) {
        if (!aggregates.empty()) {
            // 有聚合函数时，使用非聚合项（聚合结果）
            current = std::make_shared<ProjectOperator>(current, nonAggregateItems);
        } else {
            current = std::make_shared<ProjectOperator>(current, stmt->selectItems);
        }
    }

    // LIMIT/OFFSET
    if (stmt->limit >= 0 || stmt->offset > 0) {
        int limit = (stmt->limit >= 0) ? stmt->limit : -1;
        int offset = (stmt->offset >= 0) ? stmt->offset : 0;
        current = std::make_shared<LimitOperator>(current, limit, offset);
    }

    return current;
}

} // namespace executor
} // namespace minisql
