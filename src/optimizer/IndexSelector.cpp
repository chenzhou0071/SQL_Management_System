#include "IndexSelector.h"
#include "Statistics.h"
#include "../common/Logger.h"
#include "../storage/IndexManager.h"
#include "../storage/TableManager.h"
#include "../parser/AST.h"
#include <algorithm>
#include <optional>
#include <cmath>

using namespace minisql;
using namespace optimizer;

Result<std::optional<IndexMatch>> IndexSelector::selectIndex(
    const std::string& dbName,
    const std::string& tableName,
    parser::ExprPtr whereClause) {

    auto conditions = extractConditions(whereClause);
    return selectIndex(dbName, tableName, conditions);
}

Result<std::optional<IndexMatch>> IndexSelector::selectIndex(
    const std::string& dbName,
    const std::string& tableName,
    const std::vector<parser::ExprPtr>& conditions) {

    // 获取所有可用索引
    auto indexesResult = getAvailableIndexes(dbName, tableName);
    if (indexesResult.isError()) {
        return Result<std::optional<IndexMatch>>(indexesResult.getError());
    }
    // 必须使用值，不能使用引用，因为 indexesResult 会在语句结束时销毁
    auto indexes = *indexesResult.getValue();

    if (indexes.empty()) {
        return Result<std::optional<IndexMatch>>(std::nullopt);
    }

    // 提取查询涉及的列
    std::vector<std::string> queryColumns;
    for (const auto& cond : conditions) {
        auto cols = extractColumns(cond);
        queryColumns.insert(queryColumns.end(), cols.begin(), cols.end());
    }

    // 评估每个索引
    IndexMatch bestMatch;
    bestMatch.selectivity = 1.0;  // 默认最差选择性
    bestMatch.matchedColumns = 0;
    bool hasMatch = false;

    for (const auto& index : indexes) {
        auto match = matchIndex(index, tableName, conditions, queryColumns);
        if (match.matchedColumns > 0) {
            match.selectivity = calculateSelectivity(dbName, tableName, match);
            // 选择匹配列最多且选择性最低的索引
            if (!hasMatch ||
                match.matchedColumns > bestMatch.matchedColumns ||
                (match.matchedColumns == bestMatch.matchedColumns &&
                 match.selectivity < bestMatch.selectivity)) {
                bestMatch = match;
                hasMatch = true;
            }
        }
    }

    if (!hasMatch) {
        return Result<std::optional<IndexMatch>>(std::nullopt);
    }

    return Result<std::optional<IndexMatch>>(bestMatch);
}

Result<std::vector<IndexDef>> IndexSelector::getAvailableIndexes(
    const std::string& dbName,
    const std::string& tableName) {

    // 获取表定义以获取索引信息
    auto tableDefResult = storage::TableManager::getInstance().getTableDef(dbName, tableName);
    if (tableDefResult.isError()) {
        return Result<std::vector<IndexDef>>(tableDefResult.getError());
    }
    auto& tableDef = *tableDefResult.getValue();

    // 返回副本，避免悬空引用
    return Result<std::vector<IndexDef>>(tableDef.indexes);
}

std::vector<std::string> IndexSelector::extractColumns(parser::ExprPtr expr) {
    std::vector<std::string> columns;

    if (!expr) return columns;

    if (expr->getType() == parser::ASTNodeType::COLUMN_REF) {
        auto colRef = std::dynamic_pointer_cast<parser::ColumnRef>(expr);
        if (colRef) {
            columns.push_back(colRef->getColumn());
        }
    } else if (expr->getType() == parser::ASTNodeType::BINARY_EXPR) {
        auto binExpr = std::dynamic_pointer_cast<parser::BinaryExpr>(expr);
        if (binExpr) {
            auto leftCols = extractColumns(binExpr->getLeft());
            auto rightCols = extractColumns(binExpr->getRight());
            columns.insert(columns.end(), leftCols.begin(), leftCols.end());
            columns.insert(columns.end(), rightCols.begin(), rightCols.end());
        }
    } else if (expr->getType() == parser::ASTNodeType::UNARY_EXPR) {
        auto unaryExpr = std::dynamic_pointer_cast<parser::UnaryExpr>(expr);
        if (unaryExpr) {
            columns = extractColumns(unaryExpr->getOperand());
        }
    }

    return columns;
}

std::vector<parser::ExprPtr> IndexSelector::extractConditions(parser::ExprPtr expr) {
    std::vector<parser::ExprPtr> conditions;

    if (!expr) return conditions;

    if (expr->getType() == parser::ASTNodeType::BINARY_EXPR) {
        auto binExpr = std::dynamic_pointer_cast<parser::BinaryExpr>(expr);
        if (binExpr) {
            std::string op = binExpr->getOp();
            // AND 条件下拆分为独立条件
            std::transform(op.begin(), op.end(), op.begin(), ::toupper);
            if (op == "AND") {
                auto leftConds = extractConditions(binExpr->getLeft());
                auto rightConds = extractConditions(binExpr->getRight());
                conditions.insert(conditions.end(), leftConds.begin(), leftConds.end());
                conditions.insert(conditions.end(), rightConds.begin(), rightConds.end());
            } else {
                conditions.push_back(expr);
            }
        }
    } else {
        conditions.push_back(expr);
    }

    return conditions;
}

IndexMatch IndexSelector::matchIndex(
    const IndexDef& index,
    const std::string& tableName,
    const std::vector<parser::ExprPtr>& conditions,
    const std::vector<std::string>& queryColumns) {

    IndexMatch match;
    match.indexName = index.name;
    match.tableName = tableName;
    match.columns = index.columns;
    match.matchedColumns = 0;
    match.hasEquality = false;
    match.isCovering = false;

    // 检查复合索引的最左前缀匹配
    for (size_t i = 0; i < index.columns.size(); ++i) {
        const std::string& idxCol = index.columns[i];

        for (const auto& cond : conditions) {
            if (!isEqualityCondition(cond) && i > 0) {
                // 范围条件只能在索引的第一列或等值条件之后
                continue;
            }

            auto condCol = getConditionColumn(cond);
            if (condCol && *condCol == idxCol) {
                match.matchedColumns++;

                if (isEqualityCondition(cond)) {
                    match.hasEquality = true;
                }

                // 如果所有索引列都被覆盖，则为覆盖索引
                if (match.matchedColumns == static_cast<int>(index.columns.size())) {
                    // 检查 SELECT 中的列是否都被索引覆盖
                    bool allCovered = true;
                    for (const auto& queryCol : queryColumns) {
                        if (std::find(index.columns.begin(), index.columns.end(), queryCol) == index.columns.end()) {
                            allCovered = false;
                            break;
                        }
                    }
                    match.isCovering = allCovered;
                }
                break;
            }
        }
    }

    return match;
}

double IndexSelector::calculateSelectivity(
    const std::string& dbName,
    const std::string& tableName,
    const IndexMatch& match) {

    if (match.columns.empty()) {
        return 1.0;
    }

    // 使用第一个匹配的列来估算选择性
    const std::string& firstCol = match.columns[0];
    double selectivity = Statistics::getInstance().estimateSelectivity(dbName, tableName, firstCol);

    // 如果是复合索引且有等值条件，进一步降低选择性
    if (match.hasEquality && match.matchedColumns > 1) {
        selectivity = std::pow(selectivity, match.matchedColumns);
    }

    return selectivity;
}

bool IndexSelector::isEqualityCondition(const parser::ExprPtr& expr) {
    if (!expr || expr->getType() != parser::ASTNodeType::BINARY_EXPR) {
        return false;
    }

    auto binExpr = std::dynamic_pointer_cast<parser::BinaryExpr>(expr);
    if (!binExpr) return false;

    std::string op = binExpr->getOp();
    std::transform(op.begin(), op.end(), op.begin(), ::toupper);
    return op == "=" || op == "IS";
}

std::optional<std::string> IndexSelector::getConditionColumn(const parser::ExprPtr& expr) {
    if (!expr || expr->getType() != parser::ASTNodeType::BINARY_EXPR) {
        return std::nullopt;
    }

    auto binExpr = std::dynamic_pointer_cast<parser::BinaryExpr>(expr);
    if (!binExpr) return std::nullopt;

    // 检查左操作数是否是列引用
    auto left = binExpr->getLeft();
    if (left && left->getType() == parser::ASTNodeType::COLUMN_REF) {
        auto colRef = std::dynamic_pointer_cast<parser::ColumnRef>(left);
        if (colRef) {
            return colRef->getColumn();
        }
    }

    return std::nullopt;
}

bool IndexSelector::isIndexUsable(const std::vector<std::string>& indexColumns,
                                 const std::vector<std::string>& queryColumns) {
    // 检查查询列是否与索引列匹配
    for (const auto& queryCol : queryColumns) {
        if (std::find(indexColumns.begin(), indexColumns.end(), queryCol) == indexColumns.end()) {
            return false;
        }
    }
    return true;
}
