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

// ============================================================
// 代价优化器 - 基于代价模型选择最优执行计划
// ============================================================
// 代价模型原理：
// 代价 = IO代价 + CPU代价
// - IO代价：磁盘访问次数（扫描的页数）
// - CPU代价：计算开销（处理行数 × 单行代价系数）
//
// 代价估算策略：
// 1. TableScan：代价 = 表页数 + 行数 × CPU系数
// 2. Filter：代价 = 子节点代价 + 过滤行数 × CPU系数
// 3. Join：代价 = 外表代价 + 外表行数 × 内表代价（嵌套循环）
// 4. Sort：代价 = 子节点代价 + n×log(n) × CPU系数
// 5. Aggregate：代价 = 子节点代价 + 行数 × CPU系数
//
// 注意：当前实现为简化版本，主要用于演示代价计算原理
//       实际数据库会使用更精确的统计信息和代价公式
// ============================================================

// ============================================================
// 代价优化主函数
// ============================================================
// 当前实现：直接返回输入计划（简化版本）
// 完整实现应：
// 1. 生成多个候选执行计划（不同索引、不同连接顺序）
// 2. 对每个计划估算代价
// 3. 选择代价最低的计划
// ============================================================
Result<executor::OperatorPtr> CostOptimizer::optimize(
    const std::string& dbName,
    executor::OperatorPtr plan) {
    // 代价优化器目前作为规则优化器的补充
    // 主要用于多计划选择时的代价估算
    LOG_INFO("CostOptimizer", "Optimizing with cost model");
    return Result<executor::OperatorPtr>(plan);
}

// ============================================================
// 代价估算总入口
// ============================================================
// 递归估算执行算子的代价
// 根据算子类型调用对应的代价估算函数
// ============================================================
CostEstimate CostOptimizer::estimateCost(
    const std::string& dbName,
    executor::OperatorPtr node) {

    if (!node) {
        return CostEstimate();
    }

    std::string opType = getOperatorType(node);

    // 根据算子类型调用对应的代价估算函数
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

// ============================================================
// 表扫描代价估算
// ============================================================
// 代价公式：
// IO代价 = 扫描的表页数
// CPU代价 = 过滤后的行数 × CPU_TABLE_SCAN系数
// 结果行数 = 总行数 × 选择性
//
// 计算步骤：
// 1. 从统计信息获取表行数和大小
// 2. 计算表页数 = 表大小 / 页大小
// 3. 根据过滤条件估算选择性
// 4. 计算过滤后行数和CPU代价
// ============================================================
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
        // 估算页数：表大小 / 页大小（默认4KB）
        tablePages = std::max<int64_t>(1, statsResult.getValue()->tableSize / CostModel::PAGE_SIZE);
    }

    // 选择性估算：过滤条件能过滤掉多少数据
    double selectivity = 1.0;  // 默认无过滤，扫描全部数据
    if (filter) {
        selectivity = Statistics::getInstance().estimateSelectivity(dbName, tableName, "");
    }

    // IO代价：扫描的页数（假设顺序扫描，无随机IO）
    double ioCost = static_cast<double>(tablePages);

    // CPU代价：处理的行数 × 单行扫描代价
    int64_t filteredRows = static_cast<int64_t>(rowCount * selectivity);
    double cpuCost = filteredRows * CostModel::CPU_TABLE_SCAN;

    return CostEstimate(cpuCost, ioCost, filteredRows);
}

// ============================================================
// 连接代价估算（嵌套循环连接）
// ============================================================
// 代价公式：
// CPU代价 = 外表行数 × 内表行数 × CPU_NESTED_LOOP系数
// IO代价 = 外表代价 + 外表行数 × 内表代价
// 结果行数 = 外表行数 × 内表行数 / 100（粗略估算）
//
// 嵌套循环连接原理：
// - 外表的每一行，扫描内表的所有行
// - 如果内表有索引，内表代价会降低（索引查找代替全表扫描）
// ============================================================
CostEstimate CostOptimizer::estimateJoinCost(
    const std::string& dbName,
    executor::OperatorPtr left,
    executor::OperatorPtr right,
    parser::ExprPtr condition) {

    // 估算左右子树的代价
    CostEstimate leftCost = estimateCost(dbName, left);
    CostEstimate rightCost = estimateCost(dbName, right);

    // 嵌套循环连接代价 = 外表行数 × 内表扫描代价
    // 假设左表为外表，右表为内表
    double cpuCost = leftCost.estimatedRows * rightCost.estimatedRows * CostModel::CPU_NESTED_LOOP;
    double ioCost = leftCost.ioCost + leftCost.estimatedRows * rightCost.ioCost;

    // 结果行数粗略估算（假设连接条件过滤掉大部分数据）
    // 实际应根据连接条件的类型和列的基数来估算
    int64_t resultRows = leftCost.estimatedRows * rightCost.estimatedRows / 100;

    return CostEstimate(cpuCost, ioCost, resultRows);
}

// ============================================================
// 过滤代价估算
// ============================================================
// 代价公式：
// CPU代价 = 子节点CPU代价 + 处理行数 × CPU_FILTER系数
// IO代价 = 子节点IO代价（过滤不增加额外IO）
// 结果行数 = 子节点行数 × 选择性
//
// 注意：过滤操作通常在内存中完成，不产生额外IO
// ============================================================
CostEstimate CostOptimizer::estimateFilterCost(
    const std::string& dbName,
    executor::OperatorPtr child,
    parser::ExprPtr condition) {

    // 估算子节点代价
    CostEstimate childCost = estimateCost(dbName, child);

    // 过滤的CPU代价：对每一行应用过滤条件
    double cpuCost = childCost.estimatedRows * CostModel::CPU_FILTER;

    // 过滤后的代价 = 子节点代价 + 过滤CPU代价
    return CostEstimate(cpuCost + childCost.cpuCost, childCost.ioCost, childCost.estimatedRows);
}

// ============================================================
// 排序代价估算
// ============================================================
// 代价公式：
// CPU代价 = 子节点CPU代价 + n×log₂(n) × CPU_SORT系数
// IO代价 = 子节点IO代价（假设内存排序，无额外IO）
// 结果行数 = 子节点行数（排序不改变行数）
//
// 排序算法复杂度：O(n log n)
// - 快速排序、归并排序的平均复杂度
// - 如果数据量大，可能需要外部排序（增加IO代价）
// ============================================================
CostEstimate CostOptimizer::estimateSortCost(
    const std::string& dbName,
    executor::OperatorPtr child,
    const std::vector<parser::OrderByItem>& orderBy) {

    // 估算子节点代价
    CostEstimate childCost = estimateCost(dbName, child);

    // 排序代价：n × log(n) × 单行排序代价系数
    int64_t rows = childCost.estimatedRows;
    double sortCost = rows * std::log2(static_cast<double>(rows + 1)) * CostModel::CPU_SORT;

    // 总代价 = 子节点代价 + 排序代价
    return CostEstimate(childCost.cpuCost + sortCost, childCost.ioCost, rows);
}

// ============================================================
// 聚合代价估算
// ============================================================
// 代价公式：
// CPU代价 = 子节点CPU代价 + 处理行数 × CPU_AGGREGATE系数
// IO代价 = 子节点IO代价（聚合在内存中完成）
// 结果行数 = 子节点行数 / 10（假设分组后减少到10%）
//
// 注意：
// - 结果行数的估算非常简化（假设每个分组平均10行）
// - 实际应根据GROUP BY列的基数来估算分组数
// ============================================================
CostEstimate CostOptimizer::estimateAggregateCost(
    const std::string& dbName,
    executor::OperatorPtr child,
    const std::vector<parser::ExprPtr>& groupBy) {

    // 估算子节点代价
    CostEstimate childCost = estimateCost(dbName, child);

    // 聚合代价：对每一行计算聚合函数
    double aggCost = childCost.estimatedRows * CostModel::CPU_AGGREGATE;

    // 假设分组后行数减少到原来的10%（简化估算）
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
