#pragma once

#include "../common/Error.h"
#include "../parser/AST.h"
#include <string>
#include <memory>

namespace minisql {
namespace optimizer {

// ============================================================
// 查询重写器
// ============================================================
class QueryRewriter {
public:
    // 重写 SELECT 语句
    static std::shared_ptr<parser::SelectStmt> rewrite(parser::SelectStmt* stmt);

private:
    // 常量折叠：将编译时常量表达式计算出来
    static Result<parser::ExprPtr> foldConstants(parser::ExprPtr expr);

    // 消除冗余条件
    // 例如: age > 18 OR age = 20 -> age > 18
    static Result<parser::ExprPtr> eliminateRedundantConditions(parser::ExprPtr expr);

    // 谓词下推：将过滤条件下推到更早的阶段执行
    static Result<void> pushDownPredicates(parser::SelectStmt* stmt);

    // 简化布尔表达式
    static Result<parser::ExprPtr> simplifyBooleanExpr(parser::ExprPtr expr);

    // 辅助函数：检查表达式是否恒为真
    static bool isAlwaysTrue(parser::ExprPtr expr);

    // 辅助函数：检查表达式是否恒为假
    static bool isAlwaysFalse(parser::ExprPtr expr);

    // 辅助函数：提取列引用
    static std::string extractColumnName(parser::ExprPtr expr);
};

}  // namespace optimizer
}  // namespace minisql
