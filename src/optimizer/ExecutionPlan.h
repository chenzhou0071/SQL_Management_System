#pragma once

#include "../parser/AST.h"
#include <string>
#include <vector>
#include <memory>
#include <sstream>

namespace minisql {
namespace optimizer {

// ============================================================
// 扫描类型
// ============================================================
enum class ScanType {
    FULL_SCAN,      // 全表扫描
    INDEX_SCAN,     // 索引扫描
    COVERING_SCAN   // 覆盖索引扫描（不需要回表）
};

// ============================================================
// 计划节点
// ============================================================
struct PlanNode {
    std::string nodeId;
    std::string operatorType;    // "TableScan", "IndexScan", "NestedLoopJoin", etc.
    ScanType scanType = ScanType::FULL_SCAN;
    std::string tableName;
    std::string indexName;       // 使用的索引
    std::string joinType;        // JOIN 类型: INNER, LEFT, RIGHT, FULL
    std::vector<std::string> columns;  // 涉及的列
    parser::ExprPtr condition;   // 过滤条件
    std::vector<parser::ExprPtr> projections;  // 投影列（用于 ProjectOperator）
    std::vector<parser::OrderByItem> orderBy;  // 排序项（用于 SortOperator）
    std::vector<parser::ExprPtr> groupBy;  // 分组列（用于 AggregateOperator）
    parser::ExprPtr havingClause;  // HAVING 条件（用于 AggregateOperator）
    int limit = -1;  // LIMIT 值
    int offset = 0;  // OFFSET 值
    double cost = 0;            // 预估代价
    int64_t estimatedRows = 0;  // 预估行数
    std::vector<std::shared_ptr<PlanNode>> children;  // 子节点

    // 转换为字符串表示
    std::string toString() const {
        std::ostringstream oss;
        oss << nodeId << ": " << operatorType;
        if (!tableName.empty()) {
            oss << " on " << tableName;
        }
        if (!indexName.empty()) {
            oss << " using " << indexName;
        }
        if (estimatedRows > 0) {
            oss << " (rows=" << estimatedRows << ")";
        }
        for (const auto& child : children) {
            oss << "\n  -> " << child->toString();
        }
        return oss.str();
    }
};

}  // namespace optimizer
}  // namespace minisql
