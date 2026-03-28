#include "QueryOptimizer.h"
#include "QueryRewriter.h"
#include "RuleOptimizer.h"
#include "CostOptimizer.h"
#include "PlanGenerator.h"
#include "../common/Logger.h"
#include <sstream>

using namespace minisql;
using namespace optimizer;

QueryOptimizer& QueryOptimizer::getInstance() {
    static QueryOptimizer instance;
    return instance;
}

QueryOptimizer::QueryOptimizer() {
    LOG_INFO("QueryOptimizer", "QueryOptimizer initialized");
}

Result<executor::OperatorPtr> QueryOptimizer::optimize(
    const std::string& dbName,
    parser::SelectStmt* stmt) {

    if (!stmt) {
        MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "NULL statement");
        return Result<executor::OperatorPtr>(error);
    }

    LOG_INFO("QueryOptimizer", "Starting query optimization");

    // ========== 步骤 1: 查询重写 ==========
    LOG_DEBUG("QueryOptimizer", "Step 1: Query rewriting");
    auto rewrittenStmt = QueryRewriter::rewrite(stmt);
    if (!rewrittenStmt) {
        LOG_WARN("QueryOptimizer", "Query rewriting failed, using original statement");
        rewrittenStmt = std::make_shared<parser::SelectStmt>(*stmt);
    }

    // ========== 步骤 2: 生成逻辑计划 ==========
    LOG_DEBUG("QueryOptimizer", "Step 2: Generating logical plan");
    auto logicalPlanResult = PlanGenerator::generate(dbName, rewrittenStmt.get());
    if (logicalPlanResult.isError()) {
        return Result<executor::OperatorPtr>(logicalPlanResult.getError());
    }
    auto logicalPlan = *logicalPlanResult.getValue();

    LOG_DEBUG("QueryOptimizer", "Logical plan: " + logicalPlan->toString());

    // ========== 步骤 3: 生成物理计划 ==========
    LOG_DEBUG("QueryOptimizer", "Step 3: Materializing physical plan");
    auto physicalPlanResult = PlanGenerator::materialize(dbName, logicalPlan);
    if (physicalPlanResult.isError()) {
        return Result<executor::OperatorPtr>(physicalPlanResult.getError());
    }
    auto physicalPlan = *physicalPlanResult.getValue();

    // ========== 步骤 4: 规则优化 ==========
    LOG_DEBUG("QueryOptimizer", "Step 4: Applying rule-based optimizations");
    auto ruleOptimizedPlan = RuleOptimizer::optimize(physicalPlan);
    if (!ruleOptimizedPlan.isSuccess()) {
        LOG_WARN("QueryOptimizer", "Rule optimization failed, using unoptimized plan");
    } else {
        physicalPlan = *ruleOptimizedPlan.getValue();
    }

    // ========== 步骤 5: 代价优化 ==========
    LOG_DEBUG("QueryOptimizer", "Step 5: Applying cost-based optimizations");
    auto costOptimizedPlan = CostOptimizer::optimize(dbName, physicalPlan);
    if (!costOptimizedPlan.isSuccess()) {
        LOG_WARN("QueryOptimizer", "Cost optimization failed, using unoptimized plan");
    } else {
        physicalPlan = *costOptimizedPlan.getValue();
    }

    LOG_INFO("QueryOptimizer", "Query optimization completed");
    return Result<executor::OperatorPtr>(physicalPlan);
}

Result<std::shared_ptr<PlanNode>> QueryOptimizer::getLogicalPlan(
    const std::string& dbName,
    parser::SelectStmt* stmt) {

    if (!stmt) {
        MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "NULL statement");
        return Result<std::shared_ptr<PlanNode>>(error);
    }

    // 重写查询
    auto rewrittenStmt = QueryRewriter::rewrite(stmt);
    if (!rewrittenStmt) {
        rewrittenStmt = std::make_shared<parser::SelectStmt>(*stmt);
    }

    // 生成逻辑计划
    return PlanGenerator::generate(dbName, rewrittenStmt.get());
}

std::string QueryOptimizer::getRewrittenSQL(parser::SelectStmt* stmt) {
    if (!stmt) {
        return "";
    }

    auto rewritten = QueryRewriter::rewrite(stmt);
    if (!rewritten) {
        return "";  // 重写失败，返回空字符串
    }

    // TODO: 实现 AST 到 SQL 的转换
    // 目前只返回占位符
    return "[Rewritten query - AST to SQL conversion not implemented]";
}
