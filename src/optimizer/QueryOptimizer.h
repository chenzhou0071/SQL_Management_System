#pragma once

#include "../common/Error.h"
#include "../parser/AST.h"
#include "../executor/ExecutionOperator.h"
#include "ExecutionPlan.h"
#include <string>
#include <memory>

namespace minisql {
namespace optimizer {

// ============================================================
// 查询优化器 - 统一优化入口
//
// 优化流程:
// 1. QueryRewriter   - 查询重写（谓词下推、常量折叠、冗余消除）
// 2. RuleOptimizer   - 规则优化（索引使用、连接顺序、排序优化）
// 3. CostOptimizer   - 代价优化（选择最优访问路径）
// 4. PlanGenerator   - 计划生成（生成物理执行计划）
// ============================================================
class QueryOptimizer {
public:
    // 获取单例实例
    static QueryOptimizer& getInstance();

    // 优化 SELECT 语句并返回物理执行计划
    // 返回: 执行算子（可直接执行）
    static Result<executor::OperatorPtr> optimize(
        const std::string& dbName,
        parser::SelectStmt* stmt);

    // 获取逻辑执行计划（用于 EXPLAIN）
    static Result<std::shared_ptr<PlanNode>> getLogicalPlan(
        const std::string& dbName,
        parser::SelectStmt* stmt);

    // 获取优化后的 SQL（用于调试）
    static std::string getRewrittenSQL(parser::SelectStmt* stmt);

private:
    QueryOptimizer();
    ~QueryOptimizer() = default;
    QueryOptimizer(const QueryOptimizer&) = delete;
    QueryOptimizer& operator=(const QueryOptimizer&) = delete;
};

}  // namespace optimizer
}  // namespace minisql
