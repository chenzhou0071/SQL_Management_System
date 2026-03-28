#include "PlanGenerator.h"
#include "ExecutionPlan.h"
#include "IndexSelector.h"
#include "Statistics.h"
#include "CostOptimizer.h"
#include "QueryRewriter.h"
#include "../common/Logger.h"
#include "../executor/TableScanOperator.h"
#include "../executor/FilterOperator.h"
#include "../executor/ProjectOperator.h"
#include "../executor/NestedLoopJoinOperator.h"
#include "../executor/SubqueryScanOperator.h"
#include "../executor/SortOperator.h"
#include "../executor/AggregateOperator.h"
#include "../executor/LimitOperator.h"
#include "../executor/ExpressionEvaluator.h"
#include <algorithm>
#include <cmath>

using namespace minisql;
using namespace optimizer;

int PlanGenerator::nodeIdCounter_ = 0;

std::string PlanGenerator::generateNodeId() {
    return "id_" + std::to_string(++nodeIdCounter_);
}

std::string PlanGenerator::getAccessType(ScanType scanType, const std::string& indexName) {
    switch (scanType) {
        case ScanType::FULL_SCAN:
            return "ALL";
        case ScanType::COVERING_SCAN:
            return indexName.empty() ? "index" : "ref";
        case ScanType::INDEX_SCAN:
            return indexName.empty() ? "range" : "range";
        default:
            return "ALL";
    }
}

Result<std::shared_ptr<PlanNode>> PlanGenerator::generate(
    const std::string& dbName,
    parser::SelectStmt* stmt) {

    if (!stmt) {
        return Result<std::shared_ptr<PlanNode>>(MiniSQLException(
            ErrorCode::EXEC_INVALID_VALUE, "NULL statement"));
    }

    // 1. 查询重写 - 禁用（已在 QueryOptimizer 中处理）
    // auto rewritten = QueryRewriter::rewrite(stmt);
    // parser::SelectStmt* selectStmt = rewritten ? rewritten.get() : stmt;
    parser::SelectStmt* selectStmt = stmt;

    LOG_DEBUG("PlanGenerator", "Starting plan generation");
    // 2. 生成扫描节点（传入 WHERE 条件以支持索引选择）
    std::shared_ptr<PlanNode> current;

    if (selectStmt->fromTable && selectStmt->fromTable->subquery) {
        // FROM 子查询：递归生成子查询计划
        auto subplan = generate(dbName, selectStmt->fromTable->subquery.get());
        if (subplan.isError()) {
            return Result<std::shared_ptr<PlanNode>>(subplan.getError());
        }

        // 创建子查询扫描节点
        current = std::make_shared<PlanNode>();
        current->nodeId = generateNodeId();
        current->operatorType = "SubqueryScan";
        current->tableName = selectStmt->fromTable->alias;  // 使用别名作为表名
        current->children.push_back(*subplan.getValue());
        current->estimatedRows = (*subplan.getValue())->estimatedRows;
        current->cost = (*subplan.getValue())->cost;
    } else {
        // 普通表扫描
        std::string tableName = selectStmt->fromTable ? selectStmt->fromTable->name : "";
        auto scanResult = generateScanNode(dbName, tableName, selectStmt->whereClause);
        if (scanResult.isError()) {
            return Result<std::shared_ptr<PlanNode>>(scanResult.getError());
        }
        current = *scanResult.getValue();
    }

    // 2.3 处理 JOIN 节点（在 WHERE 之前串联）
    for (const auto& join : selectStmt->joins) {
        // 为右表生成扫描节点
        std::string rightTableName = join->table ? join->table->name : "";
        auto rightScanResult = generateScanNode(dbName, rightTableName, nullptr);
        if (rightScanResult.isError()) {
            return Result<std::shared_ptr<PlanNode>>(rightScanResult.getError());
        }
        auto rightScan = *rightScanResult.getValue();

        // 生成 JOIN 节点
        auto joinNode = std::make_shared<PlanNode>();
        joinNode->nodeId = generateNodeId();
        joinNode->operatorType = "NestedLoopJoin";
        joinNode->joinType = join->joinType;
        joinNode->condition = join->onCondition;
        joinNode->children = {current, rightScan};
        joinNode->estimatedRows = (current->estimatedRows * rightScan->estimatedRows) / 10;  // 简单估算
        joinNode->cost = current->cost + rightScan->cost + joinNode->estimatedRows * CostModel::CPU_NESTED_LOOP;
        current = joinNode;
    }

    // 2.5 创建 Filter 节点处理 WHERE 条件
    LOG_DEBUG("PlanGenerator", "Checking WHERE clause");
    if (selectStmt->whereClause) {
        LOG_DEBUG("PlanGenerator", "Creating Filter node");
        auto filterNode = std::make_shared<PlanNode>();
        filterNode->nodeId = generateNodeId();
        filterNode->operatorType = "Filter";
        filterNode->condition = selectStmt->whereClause;
        filterNode->children.push_back(current);
        filterNode->estimatedRows = current->estimatedRows / 2;  // 简单估算
        filterNode->cost = current->cost + filterNode->estimatedRows * CostModel::CPU_FILTER;
        current = filterNode;
    }

    // 3. GROUP BY + 聚合
    LOG_DEBUG("PlanGenerator", "Checking GROUP BY/aggregate");
    if (!selectStmt->groupBy.empty() || !selectStmt->selectItems.empty()) {
        // 检查是否有聚合函数
        bool hasAggregate = false;
        for (const auto& item : selectStmt->selectItems) {
            if (item->getType() == parser::ASTNodeType::FUNCTION_CALL) {
                hasAggregate = true;
                break;
            }
        }
        if (hasAggregate || !selectStmt->groupBy.empty()) {
            auto aggResult = generateAggregateNode(dbName, current, selectStmt->groupBy, selectStmt->havingClause);
            if (aggResult.isSuccess()) {
                current = *aggResult.getValue();
            }
        }
    }

    // 4. ORDER BY 排序
    if (!selectStmt->orderBy.empty()) {
        auto sortResult = generateSortNode(dbName, current, selectStmt->orderBy);
        if (sortResult.isSuccess()) {
            current = *sortResult.getValue();
        }
    }

    // 5. SELECT 投影
    LOG_DEBUG("PlanGenerator", "Generating project node");
    auto projResult = generateProjectNode(dbName, current, selectStmt->selectItems);
    if (projResult.isSuccess()) {
        current = *projResult.getValue();
    }

    // 6. LIMIT/OFFSET
    LOG_DEBUG("PlanGenerator", "Checking LIMIT/OFFSET");
    if (selectStmt->limit >= 0 || selectStmt->offset > 0) {
        auto limitResult = generateLimitNode(dbName, current, selectStmt->limit, selectStmt->offset);
        if (limitResult.isSuccess()) {
            current = *limitResult.getValue();
        }
    }

    LOG_INFO("PlanGenerator", "Generated plan: " + current->toString());
    return Result<std::shared_ptr<PlanNode>>(current);
}

Result<std::shared_ptr<PlanNode>> PlanGenerator::generateScanNode(
    const std::string& dbName,
    const std::string& tableName,
    parser::ExprPtr whereClause) {

    auto node = std::make_shared<PlanNode>();
    node->nodeId = generateNodeId();
    node->operatorType = "TableScan";
    node->tableName = tableName;

    // 获取统计信息 - 使用全表行数作为基准
    auto statsResult = Statistics::getInstance().getTableStatistics(dbName, tableName);
    if (statsResult.isSuccess()) {
        node->estimatedRows = statsResult.getValue()->rowCount;
    }

    // 选择索引（使用 WHERE 条件）
    auto indexResult = IndexSelector::selectIndex(dbName, tableName, whereClause);
    if (indexResult.isSuccess()) {
        auto& optMatch = *indexResult.getValue();
        if (optMatch.has_value()) {
            IndexMatch match = *optMatch;  // 复制避免悬空引用
            node->indexName = match.indexName;
            node->scanType = match.isCovering ? ScanType::COVERING_SCAN : ScanType::INDEX_SCAN;
        }
    }

    // 估算代价 - 扫描所有行
    node->cost = node->estimatedRows * CostModel::CPU_TABLE_SCAN;

    return Result<std::shared_ptr<PlanNode>>(node);
}

Result<std::shared_ptr<PlanNode>> PlanGenerator::generateJoinNode(
    const std::string& dbName,
    std::shared_ptr<PlanNode> left,
    std::shared_ptr<PlanNode> right,
    parser::ExprPtr condition) {

    auto node = std::make_shared<PlanNode>();
    node->nodeId = generateNodeId();
    node->operatorType = "NestedLoopJoin";
    node->condition = condition;
    node->children.push_back(left);
    node->children.push_back(right);

    // 估算连接后的行数
    node->estimatedRows = (left->estimatedRows * right->estimatedRows) / 100;

    // 估算代价
    node->cost = left->estimatedRows * right->estimatedRows * CostModel::CPU_NESTED_LOOP;

    return Result<std::shared_ptr<PlanNode>>(node);
}

Result<std::shared_ptr<PlanNode>> PlanGenerator::generateAggregateNode(
    const std::string& dbName,
    std::shared_ptr<PlanNode> child,
    const std::vector<parser::ExprPtr>& groupBy,
    parser::ExprPtr havingClause) {

    auto node = std::make_shared<PlanNode>();
    node->nodeId = generateNodeId();
    node->operatorType = "Aggregate";
    node->children.push_back(child);
    node->groupBy = groupBy;
    node->havingClause = havingClause;  // 添加 HAVING 条件

    // 估算聚合后的行数（分组后通常减少）
    node->estimatedRows = std::max<int64_t>(1, child->estimatedRows / 10);

    node->cost = child->cost + node->estimatedRows * CostModel::CPU_AGGREGATE;

    return Result<std::shared_ptr<PlanNode>>(node);
}

Result<std::shared_ptr<PlanNode>> PlanGenerator::generateSortNode(
    const std::string& dbName,
    std::shared_ptr<PlanNode> child,
    const std::vector<parser::OrderByItem>& orderBy) {

    auto node = std::make_shared<PlanNode>();
    node->nodeId = generateNodeId();
    node->operatorType = "Sort";
    node->children.push_back(child);
    node->orderBy = orderBy;

    node->estimatedRows = child->estimatedRows;

    // 估算排序代价
    double sortCost = child->estimatedRows * std::log2(static_cast<double>(child->estimatedRows + 1))
                     * CostModel::CPU_SORT;
    node->cost = child->cost + sortCost;

    return Result<std::shared_ptr<PlanNode>>(node);
}

Result<std::shared_ptr<PlanNode>> PlanGenerator::generateProjectNode(
    const std::string& dbName,
    std::shared_ptr<PlanNode> child,
    const std::vector<parser::ExprPtr>& selectItems) {

    auto node = std::make_shared<PlanNode>();
    node->nodeId = generateNodeId();
    node->operatorType = "Project";
    node->children.push_back(child);
    node->projections = selectItems;

    node->estimatedRows = child->estimatedRows;
    node->cost = child->cost;

    return Result<std::shared_ptr<PlanNode>>(node);
}

Result<std::shared_ptr<PlanNode>> PlanGenerator::generateLimitNode(
    const std::string& dbName,
    std::shared_ptr<PlanNode> child,
    int limit, int offset) {

    auto node = std::make_shared<PlanNode>();
    node->nodeId = generateNodeId();
    node->operatorType = "Limit";
    node->children.push_back(child);
    node->limit = limit;
    node->offset = offset;

    // LIMIT/OFFSET 后的行数
    int effectiveLimit = (limit >= 0) ? limit : -1;

    if (effectiveLimit >= 0) {
        node->estimatedRows = std::min<int64_t>(effectiveLimit, child->estimatedRows);
    } else {
        node->estimatedRows = child->estimatedRows;
    }

    node->cost = child->cost;

    return Result<std::shared_ptr<PlanNode>>(node);
}

Result<executor::OperatorPtr> PlanGenerator::materialize(
    const std::string& dbName,
    std::shared_ptr<PlanNode> plan) {

    if (!plan) {
        return Result<executor::OperatorPtr>(MiniSQLException(
            ErrorCode::EXEC_INVALID_VALUE, "NULL plan"));
    }

    // 递归物化子节点
    std::vector<executor::OperatorPtr> childOps;
    for (const auto& child : plan->children) {
        auto childResult = materialize(dbName, child);
        if (childResult.isError()) {
            return Result<executor::OperatorPtr>(childResult.getError());
        }
        childOps.push_back(*childResult.getValue());
    }

    // 根据节点类型物化
    std::string opType = plan->operatorType;
    if (opType == "TableScan") {
        return materializeScanNode(dbName, plan);
    } else if (opType == "Filter") {
        executor::OperatorPtr child = childOps.empty() ? nullptr : childOps[0];
        return materializeFilterNode(dbName, plan, child);
    } else if (opType == "NestedLoopJoin") {
        return materializeJoinNode(dbName, plan);
    } else if (opType == "Sort") {
        executor::OperatorPtr child = childOps.empty() ? nullptr : childOps[0];
        return materializeSortNode(dbName, plan, child);
    } else if (opType == "Aggregate") {
        executor::OperatorPtr child = childOps.empty() ? nullptr : childOps[0];
        return materializeAggregateNode(dbName, plan, child);
    } else if (opType == "Project") {
        executor::OperatorPtr child = childOps.empty() ? nullptr : childOps[0];
        return materializeProjectNode(dbName, plan, child);
    } else if (opType == "Limit") {
        executor::OperatorPtr child = childOps.empty() ? nullptr : childOps[0];
        return materializeLimitNode(dbName, plan, child);
    } else if (opType == "SubqueryScan") {
        executor::OperatorPtr child = childOps.empty() ? nullptr : childOps[0];
        if (!child) {
            return Result<executor::OperatorPtr>(MiniSQLException(
                ErrorCode::EXEC_INVALID_VALUE, "SubqueryScan node without child"));
        }
        auto op = std::make_shared<executor::SubqueryScanOperator>(child, plan->tableName);
        return Result<executor::OperatorPtr>(op);
    }

    return Result<executor::OperatorPtr>(nullptr);
}

Result<executor::OperatorPtr> PlanGenerator::materializeScanNode(
    const std::string& dbName,
    std::shared_ptr<PlanNode> node) {

    auto op = std::make_shared<executor::TableScanOperator>(dbName, node->tableName);
    return Result<executor::OperatorPtr>(op);
}

Result<executor::OperatorPtr> PlanGenerator::materializeFilterNode(
    const std::string& dbName,
    std::shared_ptr<PlanNode> node,
    executor::OperatorPtr child) {

    if (!child) {
        return Result<executor::OperatorPtr>(MiniSQLException(
            ErrorCode::EXEC_INVALID_VALUE, "Filter node without child"));
    }

    auto op = std::make_shared<executor::FilterOperator>(child, node->condition);
    op->setCurrentDatabase(dbName);
    return Result<executor::OperatorPtr>(op);
}

Result<executor::OperatorPtr> PlanGenerator::materializeJoinNode(
    const std::string& dbName,
    std::shared_ptr<PlanNode> node) {

    if (node->children.size() < 2) {
        return Result<executor::OperatorPtr>(MiniSQLException(
            ErrorCode::EXEC_INVALID_VALUE, "Join node without two children"));
    }

    // 物化左右子节点
    auto leftResult = materialize(dbName, node->children[0]);
    if (leftResult.isError()) {
        return Result<executor::OperatorPtr>(leftResult.getError());
    }

    auto rightResult = materialize(dbName, node->children[1]);
    if (rightResult.isError()) {
        return Result<executor::OperatorPtr>(rightResult.getError());
    }

    auto op = std::make_shared<executor::NestedLoopJoinOperator>(
        *leftResult.getValue(), *rightResult.getValue(),
        node->condition, static_cast<executor::NestedLoopJoinOperator::JoinType>(parseJoinTypeCode(node->joinType)));

    return Result<executor::OperatorPtr>(op);
}

// 解析 JOIN 类型字符串为枚举（0=INNER, 1=LEFT, 2=RIGHT, 3=FULL）
int PlanGenerator::parseJoinTypeCode(const std::string& typeStr) {
    if (typeStr == "LEFT") return 1;
    if (typeStr == "RIGHT") return 2;
    if (typeStr == "FULL") return 3;
    return 0;  // 默认 INNER
}

Result<executor::OperatorPtr> PlanGenerator::materializeSortNode(
    const std::string& dbName,
    std::shared_ptr<PlanNode> node,
    executor::OperatorPtr child) {

    if (!child) {
        return Result<executor::OperatorPtr>(MiniSQLException(
            ErrorCode::EXEC_INVALID_VALUE, "Sort node without child"));
    }

    // 转换 parser::OrderByItem 到 executor::SortItem
    std::vector<executor::SortItem> sortItems;
    for (const auto& item : node->orderBy) {
        sortItems.emplace_back(item.expr, item.ascending);
    }

    auto op = std::make_shared<executor::SortOperator>(child, sortItems);
    return Result<executor::OperatorPtr>(op);
}

Result<executor::OperatorPtr> PlanGenerator::materializeAggregateNode(
    const std::string& dbName,
    std::shared_ptr<PlanNode> node,
    executor::OperatorPtr child) {

    if (!child) {
        return Result<executor::OperatorPtr>(MiniSQLException(
            ErrorCode::EXEC_INVALID_VALUE, "Aggregate node without child"));
    }

    // 提取聚合函数
    std::vector<executor::AggregateFunc> aggregates;
    for (const auto& item : node->projections) {
        if (item && item->getType() == parser::ASTNodeType::FUNCTION_CALL) {
            auto funcExpr = std::dynamic_pointer_cast<parser::FunctionCallExpr>(item);
            if (funcExpr) {
                std::string funcName = funcExpr->getFuncName();
                std::transform(funcName.begin(), funcName.end(), funcName.begin(), ::toupper);
                if (funcName == "COUNT" || funcName == "SUM" || funcName == "AVG" ||
                    funcName == "MIN" || funcName == "MAX") {
                    auto args = funcExpr->getArgs();
                    parser::ExprPtr arg = args.empty() ? nullptr : args[0];
                    aggregates.emplace_back(funcName, arg);
                }
            }
        }
    }

    auto op = std::make_shared<executor::AggregateOperator>(child, node->groupBy, aggregates, node->havingClause);
    return Result<executor::OperatorPtr>(op);
}

Result<executor::OperatorPtr> PlanGenerator::materializeProjectNode(
    const std::string& dbName,
    std::shared_ptr<PlanNode> node,
    executor::OperatorPtr child) {

    if (!child) {
        return Result<executor::OperatorPtr>(MiniSQLException(
            ErrorCode::EXEC_INVALID_VALUE, "Project node without child"));
    }

    auto op = std::make_shared<executor::ProjectOperator>(child, node->projections);
    return Result<executor::OperatorPtr>(op);
}

Result<executor::OperatorPtr> PlanGenerator::materializeLimitNode(
    const std::string& dbName,
    std::shared_ptr<PlanNode> node,
    executor::OperatorPtr child) {

    if (!child) {
        return Result<executor::OperatorPtr>(MiniSQLException(
            ErrorCode::EXEC_INVALID_VALUE, "Limit node without child"));
    }

    // 将 -1 转换为 0（表示没有 OFFSET）
    size_t limit = node->limit >= 0 ? static_cast<size_t>(node->limit) : SIZE_MAX;
    size_t offset = node->offset >= 0 ? static_cast<size_t>(node->offset) : 0;

    auto op = std::make_shared<executor::LimitOperator>(child, limit, offset);
    return Result<executor::OperatorPtr>(op);
}
