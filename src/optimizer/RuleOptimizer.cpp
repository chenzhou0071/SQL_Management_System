#include "RuleOptimizer.h"
#include "IndexSelector.h"
#include "Statistics.h"
#include "../common/Logger.h"
#include "../storage/TableManager.h"
#include <algorithm>

using namespace minisql;
using namespace optimizer;

// 前向声明
static executor::OperatorPtr applyIndexRules(executor::OperatorPtr plan);
static executor::OperatorPtr applyJoinOrderRules(executor::OperatorPtr plan);
static executor::OperatorPtr applySortRules(executor::OperatorPtr plan);
static executor::OperatorPtr applyAggregateRules(executor::OperatorPtr plan);
static executor::OperatorPtr applyProjectionRules(executor::OperatorPtr plan);

RuleOptimizer& RuleOptimizer::getInstance() {
    static RuleOptimizer instance;
    return instance;
}

RuleOptimizer::RuleOptimizer() {
    initRules();
}

void RuleOptimizer::initRules() {
    // 索引使用规则
    OptimizerRule r1;
    r1.name = "index_usage";
    r1.priority = 1;
    r1.apply = &applyIndexRules;
    rules_.push_back(r1);

    // 连接顺序优化
    OptimizerRule r2;
    r2.name = "join_order";
    r2.priority = 2;
    r2.apply = &applyJoinOrderRules;
    rules_.push_back(r2);

    // 排序优化
    OptimizerRule r3;
    r3.name = "sort_optimization";
    r3.priority = 3;
    r3.apply = &applySortRules;
    rules_.push_back(r3);

    // 聚合优化
    OptimizerRule r4;
    r4.name = "aggregate_optimization";
    r4.priority = 4;
    r4.apply = &applyAggregateRules;
    rules_.push_back(r4);

    // 投影优化
    OptimizerRule r5;
    r5.name = "projection_optimization";
    r5.priority = 5;
    r5.apply = &applyProjectionRules;
    rules_.push_back(r5);

    // 按优先级排序
    std::sort(rules_.begin(), rules_.end(),
        [](const OptimizerRule& a, const OptimizerRule& b) {
            return a.priority < b.priority;
        });
}

Result<executor::OperatorPtr> RuleOptimizer::optimize(executor::OperatorPtr plan) {
    auto result = getInstance().applyAllRules(plan);
    return Result<executor::OperatorPtr>(result);
}

executor::OperatorPtr RuleOptimizer::applyAllRules(executor::OperatorPtr plan) {
    if (!plan) {
        return plan;
    }

    executor::OperatorPtr current = plan;

    for (const auto& rule : getInstance().rules_) {
        LOG_INFO("RuleOptimizer", "Applying rule: " + rule.name);
        current = rule.apply(current);
    }

    return current;
}

void RuleOptimizer::addRule(const OptimizerRule& rule) {
    getInstance().rules_.push_back(rule);
    std::sort(getInstance().rules_.begin(), getInstance().rules_.end(),
        [](const OptimizerRule& a, const OptimizerRule& b) {
            return a.priority < b.priority;
        });
}

executor::OperatorPtr applyIndexRules(executor::OperatorPtr plan) {
    // TODO: 实现索引使用规则
    // 1. 检查 WHERE 条件是否可以使用索引
    // 2. 如果有索引可用，将 TableScanOperator 替换为 IndexScanOperator
    return plan;
}

executor::OperatorPtr applyJoinOrderRules(executor::OperatorPtr plan) {
    // TODO: 实现连接顺序优化
    // 1. 收集所有连接操作
    // 2. 根据表大小排序（小表驱动大表）
    // 3. 重新排列 NestedLoopJoin 的左右子节点
    return plan;
}

executor::OperatorPtr applySortRules(executor::OperatorPtr plan) {
    // TODO: 实现排序优化
    // 1. 检查是否有 ORDER BY
    // 2. 检查排序列是否有索引
    // 3. 如果有索引且索引顺序与 ORDER BY 一致，消除额外排序
    return plan;
}

executor::OperatorPtr applyAggregateRules(executor::OperatorPtr plan) {
    // TODO: 实现聚合优化
    // 1. 检查是否有 HAVING 条件
    // 2. 将可以下推到 WHERE 的条件提前
    return plan;
}

executor::OperatorPtr applyProjectionRules(executor::OperatorPtr plan) {
    // TODO: 实现投影优化
    // 1. 分析 SELECT 列表中需要的列
    // 2. 消除不必要的列读取
    return plan;
}
