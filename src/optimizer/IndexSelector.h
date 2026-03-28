#pragma once

#include "../common/Error.h"
#include "../parser/AST.h"
#include "../common/Type.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace minisql {
namespace optimizer {

// ============================================================
// 索引匹配结果
// ============================================================
struct IndexMatch {
    std::string indexName;          // 索引名称
    double selectivity;             // 选择性（0-1，越小越好）
    int matchedColumns;             // 匹配的列数
    bool hasEquality;              // 是否有等值条件
    bool isCovering;               // 是否是覆盖索引
    std::string tableName;         // 所属表名
    std::vector<std::string> columns;  // 索引列
};

// ============================================================
// 索引选择器
// ============================================================
class IndexSelector {
public:
    // 为查询选择最优索引
    // @param dbName 数据库名
    // @param tableName 表名
    // @param conditions WHERE 子句中的条件表达式
    // @return 最佳索引匹配（如果没有可用索引返回 nullopt）
    static Result<std::optional<IndexMatch>> selectIndex(
        const std::string& dbName,
        const std::string& tableName,
        parser::ExprPtr whereClause);

    // 为多个条件选择最优索引
    static Result<std::optional<IndexMatch>> selectIndex(
        const std::string& dbName,
        const std::string& tableName,
        const std::vector<parser::ExprPtr>& conditions);

    // 检查某个索引是否适合给定的查询列
    static bool isIndexUsable(const std::vector<std::string>& indexColumns,
                             const std::vector<std::string>& queryColumns);

    // 获取表上所有可用索引
    static Result<std::vector<IndexDef>> getAvailableIndexes(
        const std::string& dbName,
        const std::string& tableName);

    // 提取条件中的列引用（公开供测试使用）
    static std::vector<std::string> extractColumns(parser::ExprPtr expr);

    // 判断是否是等值条件（公开供测试使用）
    static bool isEqualityCondition(const parser::ExprPtr& expr);

    // 提取所有查询条件
    static std::vector<parser::ExprPtr> extractConditions(parser::ExprPtr expr);

private:
    // 匹配索引
    static IndexMatch matchIndex(
        const IndexDef& index,
        const std::string& tableName,
        const std::vector<parser::ExprPtr>& conditions,
        const std::vector<std::string>& queryColumns);

    // 计算 selectivity
    static double calculateSelectivity(
        const std::string& dbName,
        const std::string& tableName,
        const IndexMatch& match);

    // 获取条件对应的列名
    static std::optional<std::string> getConditionColumn(const parser::ExprPtr& expr);
};

}  // namespace optimizer
}  // namespace minisql
