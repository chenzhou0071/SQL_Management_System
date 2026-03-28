#pragma once

#include "../common/Error.h"
#include "../executor/ExecutionOperator.h"
#include <string>
#include <vector>
#include <memory>

namespace minisql {
namespace optimizer {

// ============================================================
// 优化规则函数类型
// ============================================================
using RuleApplyFunc = executor::OperatorPtr(*)(executor::OperatorPtr);

// ============================================================
// 优化规则
// ============================================================
struct OptimizerRule {
    std::string name;
    int priority;  // 优先级，数字越小优先级越高
    RuleApplyFunc apply;
};

// ============================================================
// 规则优化器
// ============================================================
class RuleOptimizer {
public:
    // 获取单例实例
    static RuleOptimizer& getInstance();

    // 优化执行计划
    static Result<executor::OperatorPtr> optimize(executor::OperatorPtr plan);

    // 添加自定义规则
    static void addRule(const OptimizerRule& rule);

private:
    RuleOptimizer();
    ~RuleOptimizer() = default;
    RuleOptimizer(const RuleOptimizer&) = delete;
    RuleOptimizer& operator=(const RuleOptimizer&) = delete;

    // 初始化默认规则
    void initRules();

    // 应用所有规则
    executor::OperatorPtr applyAllRules(executor::OperatorPtr plan);

    std::vector<OptimizerRule> rules_;
};

}  // namespace optimizer
}  // namespace minisql
