#include "CostOptimizer.h"
#include "Statistics.h"
#include "IndexSelector.h"
#include "../common/Logger.h"
#include "../storage/TableManager.h"
#include "../executor/TableScanOperator.h"
#include "../executor/FilterOperator.h"
#include "../executor/NestedLoopJoinOperator.h"
#include "../executor/SortOperator.h"
#include "../executor/AggregateOperator.h"
#include "../executor/ProjectOperator.h"
#include "../executor/LimitOperator.h"
#include <cmath>

using namespace minisql;
using namespace optimizer;

Result<executor::OperatorPtr> CostOptimizer::optimize(
    const std::string& dbName,
    executor::OperatorPtr plan) {
    // 代价优化器目前作为规则优化器的补充
    // 主要用于多计划选择时的代价估算
    LOG_INFO("CostOptimizer", "Optimizing with cost model");
    return Result<executor::OperatorPtr>(plan);
}

CostEstimate CostOptimizer::estimateCost(
    const std::string& dbName,
    executor::OperatorPtr node) {

    if (!node) {
        return CostEstimate();
    }

    std::string opType = getOperatorType(node);

    if (opType == "TableScan") {
        // 估算表扫描代价
        return estimateScanCost(dbName, "", nullptr);
    } else if (opType == "Filter") {
        // 估算过滤代价
        return estimateFilterCost(dbName, nullptr, nullptr);
    } else if (opType == "NestedLoopJoin") {
        // 估算连接代价
        return estimateJoinCost(dbName, nullptr, nullptr, nullptr);
    }

    return CostEstimate();
}

CostEstimate CostOptimizer::estimateScanCost(
    const std::string& dbName,
    const std::string& tableName,
    parser::ExprPtr filter) {

    // 获取表统计信息
    auto statsResult = Statistics::getInstance().getTableStatistics(dbName, tableName);
    int64_t rowCount = 0;
    int64_t tablePages = 1;

    if (statsResult.isSuccess()) {
        rowCount = statsResult.getValue()->rowCount;
        // 估算页数：表大小 / 页大小
        tablePages = std::max<int64_t>(1, statsResult.getValue()->tableSize / CostModel::PAGE_SIZE);
    }

    // 选择性估算
    double selectivity = 1.0;
    if (filter) {
        selectivity = Statistics::getInstance().estimateSelectivity(dbName, tableName, "");
    }

    // IO 代价：扫描的页数
    double ioCost = static_cast<double>(tablePages);

    // CPU 代价：处理的行数
    int64_t filteredRows = static_cast<int64_t>(rowCount * selectivity);
    double cpuCost = filteredRows * CostModel::CPU_TABLE_SCAN;

    return CostEstimate(cpuCost, ioCost, filteredRows);
}

CostEstimate CostOptimizer::estimateJoinCost(
    const std::string& dbName,
    executor::OperatorPtr left,
    executor::OperatorPtr right,
    parser::ExprPtr condition) {

    // 估算左右子树的代价
    CostEstimate leftCost = estimateCost(dbName, left);
    CostEstimate rightCost = estimateCost(dbName, right);

    // 嵌套循环连接代价 = 外表行数 * 内表扫描代价
    double cpuCost = leftCost.estimatedRows * rightCost.estimatedRows * CostModel::CPU_NESTED_LOOP;
    double ioCost = leftCost.ioCost + leftCost.estimatedRows * rightCost.ioCost;

    int64_t resultRows = leftCost.estimatedRows * rightCost.estimatedRows / 100;  // 粗略估算

    return CostEstimate(cpuCost, ioCost, resultRows);
}

CostEstimate CostOptimizer::estimateFilterCost(
    const std::string& dbName,
    executor::OperatorPtr child,
    parser::ExprPtr condition) {

    // 估算子节点代价
    CostEstimate childCost = estimateCost(dbName, child);

    // 过滤的 CPU 代价
    double cpuCost = childCost.estimatedRows * CostModel::CPU_FILTER;

    return CostEstimate(cpuCost + childCost.cpuCost, childCost.ioCost, childCost.estimatedRows);
}

CostEstimate CostOptimizer::estimateSortCost(
    const std::string& dbName,
    executor::OperatorPtr child,
    const std::vector<parser::OrderByItem>& orderBy) {

    // 估算子节点代价
    CostEstimate childCost = estimateCost(dbName, child);

    // 排序代价：n * log(n) * 系数
    int64_t rows = childCost.estimatedRows;
    double sortCost = rows * std::log2(static_cast<double>(rows + 1)) * CostModel::CPU_SORT;

    return CostEstimate(childCost.cpuCost + sortCost, childCost.ioCost, rows);
}

CostEstimate CostOptimizer::estimateAggregateCost(
    const std::string& dbName,
    executor::OperatorPtr child,
    const std::vector<parser::ExprPtr>& groupBy) {

    // 估算子节点代价
    CostEstimate childCost = estimateCost(dbName, child);

    // 聚合代价
    double aggCost = childCost.estimatedRows * CostModel::CPU_AGGREGATE;

    // 假设分组后行数减少到原来的 10%
    int64_t resultRows = std::max<int64_t>(1, childCost.estimatedRows / 10);

    return CostEstimate(childCost.cpuCost + aggCost, childCost.ioCost, resultRows);
}

double CostOptimizer::estimateSelectivity(
    const std::string& dbName,
    const std::string& tableName,
    parser::ExprPtr condition) {
    return Statistics::getInstance().estimateSelectivity(dbName, tableName, "");
}

std::string CostOptimizer::getOperatorType(executor::OperatorPtr op) {
    if (!op) return "null";

    // 使用 RTTI 获取类型名称
    if (dynamic_cast<executor::TableScanOperator*>(op.get())) {
        return "TableScan";
    } else if (dynamic_cast<executor::FilterOperator*>(op.get())) {
        return "Filter";
    } else if (dynamic_cast<executor::NestedLoopJoinOperator*>(op.get())) {
        return "NestedLoopJoin";
    } else if (dynamic_cast<executor::SortOperator*>(op.get())) {
        return "Sort";
    } else if (dynamic_cast<executor::AggregateOperator*>(op.get())) {
        return "Aggregate";
    } else if (dynamic_cast<executor::ProjectOperator*>(op.get())) {
        return "Project";
    } else if (dynamic_cast<executor::LimitOperator*>(op.get())) {
        return "Limit";
    }

    return "Unknown";
}
