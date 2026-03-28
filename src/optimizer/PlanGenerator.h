#pragma once

#include "../common/Error.h"
#include "ExecutionPlan.h"
#include "../executor/ExecutionOperator.h"
#include <string>
#include <memory>

namespace minisql {
namespace optimizer {

// ============================================================
// 计划生成器
// ============================================================
class PlanGenerator {
public:
    // 生成执行计划
    static Result<std::shared_ptr<PlanNode>> generate(
        const std::string& dbName,
        parser::SelectStmt* stmt);

    // 将逻辑计划转换为物理算子
    static Result<executor::OperatorPtr> materialize(
        const std::string& dbName,
        std::shared_ptr<PlanNode> plan);

    // 获取访问类型字符串（用于 EXPLAIN）
    static std::string getAccessType(ScanType scanType, const std::string& indexName);

private:
    // 生成扫描节点
    static Result<std::shared_ptr<PlanNode>> generateScanNode(
        const std::string& dbName,
        const std::string& tableName,
        parser::ExprPtr whereClause);

    // 生成连接节点
    static Result<std::shared_ptr<PlanNode>> generateJoinNode(
        const std::string& dbName,
        std::shared_ptr<PlanNode> left,
        std::shared_ptr<PlanNode> right,
        parser::ExprPtr condition);

    // 生成聚合节点
    static Result<std::shared_ptr<PlanNode>> generateAggregateNode(
        const std::string& dbName,
        std::shared_ptr<PlanNode> child,
        const std::vector<parser::ExprPtr>& groupBy);

    // 生成排序节点
    static Result<std::shared_ptr<PlanNode>> generateSortNode(
        const std::string& dbName,
        std::shared_ptr<PlanNode> child,
        const std::vector<parser::OrderByItem>& orderBy);

    // 生成投影节点
    static Result<std::shared_ptr<PlanNode>> generateProjectNode(
        const std::string& dbName,
        std::shared_ptr<PlanNode> child,
        const std::vector<parser::ExprPtr>& selectItems);

    // 生成限制节点
    static Result<std::shared_ptr<PlanNode>> generateLimitNode(
        const std::string& dbName,
        std::shared_ptr<PlanNode> child,
        int limit, int offset);

    // 物化扫描节点
    static Result<executor::OperatorPtr> materializeScanNode(
        const std::string& dbName,
        std::shared_ptr<PlanNode> node);

    // 物化过滤节点
    static Result<executor::OperatorPtr> materializeFilterNode(
        const std::string& dbName,
        std::shared_ptr<PlanNode> node,
        executor::OperatorPtr child);

    // 物化连接节点
    static Result<executor::OperatorPtr> materializeJoinNode(
        const std::string& dbName,
        std::shared_ptr<PlanNode> node);

    // 物化排序节点
    static Result<executor::OperatorPtr> materializeSortNode(
        const std::string& dbName,
        std::shared_ptr<PlanNode> node,
        executor::OperatorPtr child);

    // 物化聚合节点
    static Result<executor::OperatorPtr> materializeAggregateNode(
        const std::string& dbName,
        std::shared_ptr<PlanNode> node,
        executor::OperatorPtr child);

    // 物化投影节点
    static Result<executor::OperatorPtr> materializeProjectNode(
        const std::string& dbName,
        std::shared_ptr<PlanNode> node,
        executor::OperatorPtr child);

    // 物化限制节点
    static Result<executor::OperatorPtr> materializeLimitNode(
        const std::string& dbName,
        std::shared_ptr<PlanNode> node,
        executor::OperatorPtr child);

    // 生成唯一节点 ID
    static std::string generateNodeId();

    static int nodeIdCounter_;
};

}  // namespace optimizer
}  // namespace minisql
