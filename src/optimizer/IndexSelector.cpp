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

// ============================================================
// 索引选择器 - 为查询选择最优索引
// ============================================================
// 核心算法原理：
// 1. 从WHERE子句提取所有查询条件
// 2. 获取表上的所有可用索引
// 3. 对每个索引评估匹配度（最左前缀匹配）
// 4. 计算索引选择性（估算过滤后的行数比例）
// 5. 选择匹配列数最多且选择性最低的索引
//
// 索引匹配规则：
// - 复合索引必须满足最左前缀原则
// - 等值条件（=, IS）优先于范围条件（>, <, BETWEEN）
// - 范围条件只能出现在等值条件之后或索引第一列
// ============================================================

// ============================================================
// 索引选择主函数（WHERE子句版本）
// ============================================================
// 流程：
// 1. 从WHERE子句提取所有独立条件（拆解AND条件）
// 2. 调用多条件版本选择最优索引
// ============================================================
Result<std::optional<IndexMatch>> IndexSelector::selectIndex(
    const std::string& dbName,
    const std::string& tableName,
    parser::ExprPtr whereClause) {

    auto conditions = extractConditions(whereClause);
    return selectIndex(dbName, tableName, conditions);
}

// ============================================================
// 索引选择主函数（条件列表版本）
// ============================================================
// 核心算法：
// 1. 获取表上所有可用索引
// 2. 提取查询涉及的列名
// 3. 对每个索引进行匹配度评估：
//    - 检查最左前缀匹配
//    - 统计匹配的列数
//    - 标记是否有等值条件
// 4. 计算索引选择性（过滤效果）
// 5. 选择最优索引：匹配列数最多 -> 选择性最低
//
// 返回值：
//   - 有可用索引：返回最优索引的IndexMatch
//   - 无可用索引：返回nullopt（使用全表扫描）
// ============================================================
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

    // 如果没有索引，直接返回nullopt（使用全表扫描）
    if (indexes.empty()) {
        return Result<std::optional<IndexMatch>>(std::nullopt);
    }

    // 提取查询涉及的列名（用于判断是否是覆盖索引）
    std::vector<std::string> queryColumns;
    for (const auto& cond : conditions) {
        auto cols = extractColumns(cond);
        queryColumns.insert(queryColumns.end(), cols.begin(), cols.end());
    }

    // 评估每个索引，找到最佳匹配
    IndexMatch bestMatch;
    bestMatch.selectivity = 1.0;  // 默认最差选择性（100%的数据）
    bestMatch.matchedColumns = 0;
    bool hasMatch = false;

    for (const auto& index : indexes) {
        // 匹配索引与查询条件
        auto match = matchIndex(index, tableName, conditions, queryColumns);
        if (match.matchedColumns > 0) {
            // 计算选择性（估算过滤后的数据比例）
            match.selectivity = calculateSelectivity(dbName, tableName, match);
            // 选择策略：优先匹配列数多，其次选择性低
            if (!hasMatch ||
                match.matchedColumns > bestMatch.matchedColumns ||
                (match.matchedColumns == bestMatch.matchedColumns &&
                 match.selectivity < bestMatch.selectivity)) {
                bestMatch = match;
                hasMatch = true;
            }
        }
    }

    // 如果没有匹配的索引，返回nullopt
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

// ============================================================
// 提取查询条件中的列名（辅助函数）
// ============================================================
// 递归遍历表达式树，提取所有列引用
// 支持表达式类型：
//   - COLUMN_REF：直接列引用，如 "age"
//   - BINARY_EXPR：二元表达式，如 "age > 20"，提取左右两边的列
//   - UNARY_EXPR：一元表达式，如 "NOT flag"，提取操作数中的列
//
// 返回值：表达式涉及的所有列名列表
// ============================================================
std::vector<std::string> IndexSelector::extractColumns(parser::ExprPtr expr) {
    std::vector<std::string> columns;

    if (!expr) return columns;

    if (expr->getType() == parser::ASTNodeType::COLUMN_REF) {
        // 列引用节点：直接提取列名
        auto colRef = std::dynamic_pointer_cast<parser::ColumnRef>(expr);
        if (colRef) {
            columns.push_back(colRef->getColumn());
        }
    } else if (expr->getType() == parser::ASTNodeType::BINARY_EXPR) {
        // 二元表达式：递归提取左右子表达式中的列
        auto binExpr = std::dynamic_pointer_cast<parser::BinaryExpr>(expr);
        if (binExpr) {
            auto leftCols = extractColumns(binExpr->getLeft());
            auto rightCols = extractColumns(binExpr->getRight());
            columns.insert(columns.end(), leftCols.begin(), leftCols.end());
            columns.insert(columns.end(), rightCols.begin(), rightCols.end());
        }
    } else if (expr->getType() == parser::ASTNodeType::UNARY_EXPR) {
        // 一元表达式：提取操作数中的列
        auto unaryExpr = std::dynamic_pointer_cast<parser::UnaryExpr>(expr);
        if (unaryExpr) {
            columns = extractColumns(unaryExpr->getOperand());
        }
    }

    return columns;
}

// ============================================================
// 提取WHERE子句中的所有独立条件
// ============================================================
// 拆解AND连接的条件为独立条件列表
// 例如："age > 18 AND name = 'Alice' AND status = 'active'"
//       拆解为：["age > 18", "name = 'Alice'", "status = 'active'"]
//
// 作用：每个独立条件可以单独评估索引匹配度
// ============================================================
std::vector<parser::ExprPtr> IndexSelector::extractConditions(parser::ExprPtr expr) {
    std::vector<parser::ExprPtr> conditions;

    if (!expr) return conditions;

    if (expr->getType() == parser::ASTNodeType::BINARY_EXPR) {
        auto binExpr = std::dynamic_pointer_cast<parser::BinaryExpr>(expr);
        if (binExpr) {
            std::string op = binExpr->getOp();
            // AND条件拆解为独立条件
            std::transform(op.begin(), op.end(), op.begin(), ::toupper);
            if (op == "AND") {
                // 递归拆解AND的左右两边
                auto leftConds = extractConditions(binExpr->getLeft());
                auto rightConds = extractConditions(binExpr->getRight());
                conditions.insert(conditions.end(), leftConds.begin(), leftConds.end());
                conditions.insert(conditions.end(), rightConds.begin(), rightConds.end());
            } else {
                // 非AND条件，直接作为独立条件
                conditions.push_back(expr);
            }
        }
    } else {
        // 其他表达式类型，直接作为条件
        conditions.push_back(expr);
    }

    return conditions;
}

// ============================================================
// 索引匹配评估函数（核心算法）
// ============================================================
// 算法原理：
// 检查复合索引是否匹配查询条件，遵循最左前缀原则
//
// 最左前缀匹配规则：
// 1. 必须从索引的第一列开始匹配
// 2. 匹配的列必须是连续的前缀，不能跳过中间列
// 3. 等值条件（=）可以连续匹配多列
// 4. 范围条件（>, <, BETWEEN）只能匹配一列，且必须出现在：
//    - 索引的第一列
//    - 等值条件匹配完之后的下一列
//
// 示例：
//   索引 (a, b, c)
//   ✓ WHERE a=1 AND b=2        -> 匹配2列（a,b）
//   ✓ WHERE a=1 AND b>2        -> 匹配2列（a,b），b为范围
//   ✓ WHERE a>1                -> 匹配1列（a），第一列可以是范围
//   ✗ WHERE b=2                -> 不匹配（违反最左前缀）
//   ✗ WHERE a=1 AND c=3        -> 只匹配1列（a），跳过了b
//   ✗ WHERE a>1 AND b=2        -> 只匹配1列（a），范围后不能有等值
//
// 覆盖索引判定：
//   如果查询涉及的所有列都被索引覆盖，则称为覆盖索引
//   覆盖索引可以避免回表查询（无需访问数据文件）
// ============================================================
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

        // 遍历查询条件，检查是否有匹配此索引列的条件
        for (const auto& cond : conditions) {
            // 范围条件只能在索引的第一列或等值条件之后
            // 例如：(a,b,c)索引：a>1合法，b>1仅当a有等值条件时才合法
            if (!isEqualityCondition(cond) && i > 0) {
                continue;  // 跳过非第一列的范围条件
            }

            auto condCol = getConditionColumn(cond);
            if (condCol && *condCol == idxCol) {
                // 找到匹配此索引列的条件
                match.matchedColumns++;

                // 标记是否有等值条件（影响选择性计算）
                if (isEqualityCondition(cond)) {
                    match.hasEquality = true;
                }

                // 检查是否是覆盖索引
                // 如果所有索引列都被匹配，且查询列都被索引覆盖
                if (match.matchedColumns == static_cast<int>(index.columns.size())) {
                    // 检查SELECT中的列是否都被索引覆盖（无需回表）
                    bool allCovered = true;
                    for (const auto& queryCol : queryColumns) {
                        if (std::find(index.columns.begin(), index.columns.end(), queryCol) == index.columns.end()) {
                            allCovered = false;
                            break;
                        }
                    }
                    match.isCovering = allCovered;
                }
                break;  // 当前索引列已匹配，继续检查下一列
            }
        }
    }

    return match;
}

// ============================================================
// 计算索引选择性
// ============================================================
// 选择性定义：使用索引过滤后剩余数据占总数据的比例
// - 选择性 = 0.01 表示过滤掉99%的数据，只保留1%
// - 选择性越低，索引过滤效果越好
//
// 计算策略：
// 1. 使用统计信息估算第一列的选择性
// 2. 如果是复合索引且有多个等值条件，选择性按幂次递减
//    例如：2列等值匹配，选择性 = base_selectivity^2
//
// 注意：这是简化估算，实际数据库会使用更精确的统计信息
// ============================================================
double IndexSelector::calculateSelectivity(
    const std::string& dbName,
    const std::string& tableName,
    const IndexMatch& match) {

    if (match.columns.empty()) {
        return 1.0;  // 无索引列，返回最差选择性
    }

    // 使用第一个匹配的列来估算选择性（基础选择性）
    const std::string& firstCol = match.columns[0];
    double selectivity = Statistics::getInstance().estimateSelectivity(dbName, tableName, firstCol);

    // 如果是复合索引且有等值条件，选择性指数级降低
    // 原理：多个列同时满足等值条件的概率 = 各列概率的乘积
    if (match.hasEquality && match.matchedColumns > 1) {
        selectivity = std::pow(selectivity, match.matchedColumns);
    }

    return selectivity;
}

// ============================================================
// 判断是否是等值条件
// ============================================================
// 等值条件：使用=或IS操作符的条件
// 例如：age = 18, name IS NULL
//
// 等值条件的重要性：
// - 等值条件可以使用索引的点查询（效率最高）
// - 范围条件只能使用索引的范围扫描（效率较低）
// - 复合索引中，等值条件后才能继续匹配后续列
// ============================================================
bool IndexSelector::isEqualityCondition(const parser::ExprPtr& expr) {
    if (!expr || expr->getType() != parser::ASTNodeType::BINARY_EXPR) {
        return false;
    }

    auto binExpr = std::dynamic_pointer_cast<parser::BinaryExpr>(expr);
    if (!binExpr) return false;

    std::string op = binExpr->getOp();
    std::transform(op.begin(), op.end(), op.begin(), ::toupper);
    return op == "=" || op == "IS";  // 等值操作符
}

// ============================================================
// 从条件中提取列名
// ============================================================
// 提取二元表达式中左操作数的列名
// 前提：条件格式为 "列名 = 值" 或 "列名 > 值"
// ============================================================
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

// ============================================================
// 检查索引是否可用（辅助函数）
// ============================================================
// 检查查询涉及的所有列是否都被索引覆盖
// 用于判断索引是否能满足查询需求
// ============================================================
bool IndexSelector::isIndexUsable(const std::vector<std::string>& indexColumns,
                                 const std::vector<std::string>& queryColumns) {
    // 检查查询列是否与索引列匹配
    for (const auto& queryCol : queryColumns) {
        if (std::find(indexColumns.begin(), indexColumns.end(), queryCol) == indexColumns.end()) {
            return false;  // 查询列未被索引覆盖
        }
    }
    return true;  // 所有查询列都被索引覆盖
}
