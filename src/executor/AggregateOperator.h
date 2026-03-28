#pragma once

#include "ExecutionOperator.h"
#include "ExpressionEvaluator.h"
#include "../parser/AST.h"
#include "../common/Type.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <unordered_map>

namespace minisql {
namespace executor {

// 聚合函数定义
struct AggregateFunc {
    std::string name;  // COUNT, SUM, AVG, MIN, MAX
    parser::ExprPtr arg;

    AggregateFunc(const std::string& n, parser::ExprPtr a) : name(n), arg(a) {}
};

// 聚合算子 - 实现 GROUP BY 和聚合函数
class AggregateOperator : public ExecutionOperator {
public:
    // 构造函数
    AggregateOperator(OperatorPtr child,
                     const std::vector<parser::ExprPtr>& groupByExprs,
                     const std::vector<AggregateFunc>& aggregates,
                     parser::ExprPtr havingClause = nullptr);

    // 析构函数
    ~AggregateOperator() override;

    // 初始化算子
    Result<void> open() override;

    // 获取下一行 (返回聚合后的行)
    Result<std::optional<Tuple>> next() override;

    // 关闭算子
    Result<void> close() override;

    // 获取输出列名
    std::vector<std::string> getColumnNames() const override;

    // 获取输出列类型
    std::vector<DataType> getColumnTypes() const override;

    // 获取表名 (委托给子算子)
    std::string getTableName() const override;

private:
    OperatorPtr child_;
    std::vector<parser::ExprPtr> groupByExprs_;
    std::vector<AggregateFunc> aggregates_;
    parser::ExprPtr havingClause_;  // HAVING 条件
    ExpressionEvaluator evaluator_;
    bool isOpen_;

    // 分组状态
    struct GroupKey {
        std::vector<Value> keys;

        bool operator==(const GroupKey& other) const {
            return keys == other.keys;
        }
    };

    struct GroupKeyHash {
        size_t operator()(const GroupKey& k) const {
            size_t h = 0;
            for (const auto& v : k.keys) {
                if (!v.isNull()) {
                    h ^= std::hash<std::string>{}(v.toString()) + 0x9e3779b9;
                }
            }
            return h;
        }
    };

    // 单个聚合函数的状态
    struct SingleAggregateState {
        size_t count = 0;
        std::optional<double> sum;
        std::optional<Value> min;
        std::optional<Value> max;

        void addValue(const std::string& funcName, const Value& val);
        Value compute(const std::string& funcName);
    };

    // 分组状态：每个分组有多个聚合函数的状态
    struct AggregateState {
        std::vector<SingleAggregateState> states;  // 每个聚合函数一个状态

        AggregateState(size_t numAggregates) : states(numAggregates) {}
    };

    std::unordered_map<GroupKey, AggregateState, GroupKeyHash> groups_;
    std::vector<GroupKey> groupOrder_;
    size_t currentGroupIndex_;

    // 缓存的列名和类型
    std::vector<std::string> columnNames_;
    std::vector<DataType> columnTypes_;

    // 构建分组并计算聚合
    Result<void> buildGroups();

    // 计算分组键
    Result<GroupKey> computeGroupKey(const Tuple& row);

    // 构建行上下文
    RowContext buildRowContext(const Tuple& row);
};

} // namespace executor
} // namespace minisql
