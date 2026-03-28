#pragma once

#include "../common/Error.h"
#include "../parser/AST.h"
#include "../executor/ExecutionOperator.h"
#include <string>
#include <vector>
#include <memory>

namespace minisql {
namespace optimizer {

// ============================================================
// 代价估算
// ============================================================
struct CostEstimate {
    double cpuCost = 0;       // CPU 代价
    double ioCost = 0;        // IO 代价
    double totalCost = 0;     // 总代价
    int64_t estimatedRows = 0;  // 预估行数

    CostEstimate() = default;
    CostEstimate(double cpu, double io, int64_t rows)
        : cpuCost(cpu), ioCost(io), estimatedRows(rows) {
        totalCost = cpuCost + ioCost;
    }
};

// ============================================================
// 代价模型常量
// ============================================================
struct CostModel {
    // CPU 代价系数
    static constexpr double CPU_TABLE_SCAN = 0.01;      // 每行扫描代价
    static constexpr double CPU_INDEX_SCAN = 0.005;     // 索引扫描每行代价
    static constexpr double CPU_NESTED_LOOP = 0.02;    // 嵌套循环连接每对行代价
    static constexpr double CPU_FILTER = 0.01;         // 过滤每行代价
    static constexpr double CPU_SORT = 0.05;            // 排序每行代价
    static constexpr double CPU_AGGREGATE = 0.03;       // 聚合每行代价

    // 默认页大小（用于 IO 估算）
    static constexpr int PAGE_SIZE = 4096;
};

// ============================================================
// 代价优化器
// ============================================================
class CostOptimizer {
public:
    // 优化执行计划
    static Result<executor::OperatorPtr> optimize(
        const std::string& dbName,
        executor::OperatorPtr plan);

    // 估算子树代价
    static CostEstimate estimateCost(
        const std::string& dbName,
        executor::OperatorPtr node);

    // 估算扫描代价
    static CostEstimate estimateScanCost(
        const std::string& dbName,
        const std::string& tableName,
        parser::ExprPtr filter);

    // 估算连接代价
    static CostEstimate estimateJoinCost(
        const std::string& dbName,
        executor::OperatorPtr left,
        executor::OperatorPtr right,
        parser::ExprPtr condition);

    // 估算过滤代价
    static CostEstimate estimateFilterCost(
        const std::string& dbName,
        executor::OperatorPtr child,
        parser::ExprPtr condition);

    // 估算排序代价
    static CostEstimate estimateSortCost(
        const std::string& dbName,
        executor::OperatorPtr child,
        const std::vector<parser::OrderByItem>& orderBy);

    // 估算聚合代价
    static CostEstimate estimateAggregateCost(
        const std::string& dbName,
        executor::OperatorPtr child,
        const std::vector<parser::ExprPtr>& groupBy);

    // 估算 selectivity
    static double estimateSelectivity(
        const std::string& dbName,
        const std::string& tableName,
        parser::ExprPtr condition);

private:
    // 获取节点类型名称
    static std::string getOperatorType(executor::OperatorPtr op);
};

}  // namespace optimizer
}  // namespace minisql
