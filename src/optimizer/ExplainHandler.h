#pragma once

#include "ExecutionPlan.h"
#include "../common/Error.h"
#include "../parser/AST.h"
#include <string>
#include <vector>

namespace minisql {
namespace optimizer {

// ============================================================
// EXPLAIN 输出行
// ============================================================
struct ExplainRow {
    int id = 0;                          // 节点 ID
    std::string selectType = "SIMPLE";    // SIMPLE, PRIMARY, SUBQUERY, DERIVED, UNION
    std::string table;                   // 表名
    std::string type;                    // 访问类型：ALL, index, range, ref, eq_ref, const
    std::string possibleKeys;             // 可能使用的索引
    std::string key;                     // 实际使用的索引
    int keyLen = 0;                      // 索引长度
    int rows = 0;                        // 预估扫描行数
    std::vector<std::string> extra;      // 额外信息

    std::string toString() const;
};

// ============================================================
// EXPLAIN 处理器
// ============================================================
class ExplainHandler {
public:
    // 生成执行计划的 EXPLAIN 输出
    static Result<std::vector<ExplainRow>> explain(
        const std::string& dbName,
        parser::SelectStmt* stmt);

    // 生成已有计划的 EXPLAIN 输出
    static std::vector<ExplainRow> explainPlan(std::shared_ptr<PlanNode> plan);

    // 格式化表格输出
    static std::string formatTable(const std::vector<ExplainRow>& rows);

    // 格式化 JSON 输出
    static std::string formatJSON(const std::vector<ExplainRow>& rows);

    // 获取访问类型字符串
    static std::string getAccessType(ScanType scanType, bool hasEquality, double selectivity);

private:
    static void collectExplainRows(const PlanNode* node, int id, std::vector<ExplainRow>& rows);
    static std::string buildExtra(const PlanNode* node);
};

}  // namespace optimizer
}  // namespace minisql
