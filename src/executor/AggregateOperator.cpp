#include "AggregateOperator.h"
#include "../common/Logger.h"

namespace minisql {
namespace executor {

void AggregateOperator::SingleAggregateState::addValue(const std::string& funcName, const Value& val) {
    if (funcName == "COUNT") {
        count++;
    } else if (val.isNull()) {
        return; // NULL 值不参与其他聚合计算
    } else if (funcName == "SUM") {
        if (!sum.has_value()) {
            sum = 0.0;
        }
        if (val.getType() == DataType::INT || val.getType() == DataType::BIGINT) {
            *sum += val.getInt();
        } else if (val.getType() == DataType::DOUBLE) {
            *sum += val.getDouble();
        }
    } else if (funcName == "AVG") {
        // AVG 需要同时跟踪 count 和 sum
        count++;
        if (!sum.has_value()) {
            sum = 0.0;
        }
        if (val.getType() == DataType::INT || val.getType() == DataType::BIGINT) {
            *sum += val.getInt();
        } else if (val.getType() == DataType::DOUBLE) {
            *sum += val.getDouble();
        }
    } else if (funcName == "MIN") {
        if (!min.has_value()) {
            min = val;
        } else {
            if (val.compareTo(*min) < 0) {
                min = val;
            }
        }
    } else if (funcName == "MAX") {
        if (!max.has_value()) {
            max = val;
        } else {
            if (val.compareTo(*max) > 0) {
                max = val;
            }
        }
    }
}

Value AggregateOperator::SingleAggregateState::compute(const std::string& funcName) {
    if (funcName == "COUNT") {
        return Value(static_cast<int64_t>(count));
    } else if (funcName == "SUM") {
        if (!sum.has_value()) {
            return Value(); // NULL
        }
        return Value(*sum);
    } else if (funcName == "AVG") {
        if (!sum.has_value() || count == 0) {
            return Value(); // NULL
        }
        return Value(*sum / static_cast<double>(count));
    } else if (funcName == "MIN") {
        if (!min.has_value()) {
            return Value(); // NULL
        }
        return *min;
    } else if (funcName == "MAX") {
        if (!max.has_value()) {
            return Value(); // NULL
        }
        return *max;
    }
    return Value(); // Unknown function
}

AggregateOperator::AggregateOperator(OperatorPtr child,
                                   const std::vector<parser::ExprPtr>& groupByExprs,
                                   const std::vector<AggregateFunc>& aggregates)
    : child_(child), groupByExprs_(groupByExprs), aggregates_(aggregates),
      isOpen_(false), currentGroupIndex_(0) {
    children_.push_back(child);
}

AggregateOperator::~AggregateOperator() {
    if (isOpen_) {
        close();
    }
}

Result<void> AggregateOperator::open() {
    if (isOpen_) {
        return Result<void>();
    }

    // 打开子算子
    auto openResult = child_->open();
    if (!openResult.isSuccess()) {
        return openResult;
    }

    // 构建分组并计算聚合
    auto buildResult = buildGroups();
    if (!buildResult.isSuccess()) {
        return buildResult;
    }

    // 构建输出列名和类型
    // GROUP BY 列
    for (const auto& expr : groupByExprs_) {
        if (expr->getType() == parser::ASTNodeType::COLUMN_REF) {
            auto colRef = static_cast<parser::ColumnRef*>(expr.get());
            columnNames_.push_back(colRef->getColumn());
        } else {
            columnNames_.push_back("group_expr");
        }
        // 简化类型推导
        columnTypes_.push_back(DataType::UNKNOWN);
    }

    // 聚合列
    for (const auto& agg : aggregates_) {
        columnNames_.push_back(agg.name + "(...)");
        if (agg.name == "COUNT") {
            columnTypes_.push_back(DataType::BIGINT);
        } else if (agg.name == "SUM" || agg.name == "AVG") {
            columnTypes_.push_back(DataType::DOUBLE);
        } else {
            columnTypes_.push_back(DataType::UNKNOWN);
        }
    }

    isOpen_ = true;
    LOG_INFO("AggregateOperator", "Opened aggregate operator with " +
            std::to_string(groups_.size()) + " groups");
    return Result<void>();
}

Result<std::optional<Tuple>> AggregateOperator::next() {
    if (!isOpen_) {
        MiniSQLException error(ErrorCode::EXEC_TABLE_NOT_FOUND, "Operator not open");
        return Result<std::optional<Tuple>>(error);
    }

    if (currentGroupIndex_ >= groupOrder_.size()) {
        return Result<std::optional<Tuple>>(std::nullopt); // EOF
    }

    const GroupKey& groupKey = groupOrder_[currentGroupIndex_];
    auto it = groups_.find(groupKey);
    if (it == groups_.end()) {
        return Result<std::optional<Tuple>>(std::nullopt);
    }
    AggregateState& state = it->second;

    // 构建输出行
    Tuple row;

    // 添加 GROUP BY 列值
    for (const auto& val : groupKey.keys) {
        row.push_back(val);
    }

    // 添加聚合结果
    for (size_t i = 0; i < aggregates_.size(); ++i) {
        row.push_back(state.states[i].compute(aggregates_[i].name));
    }

    currentGroupIndex_++;
    return Result<std::optional<Tuple>>(row);
}

Result<void> AggregateOperator::close() {
    if (isOpen_) {
        child_->close();
        groups_.clear();
        groupOrder_.clear();
        currentGroupIndex_ = 0;
        isOpen_ = false;
        LOG_INFO("AggregateOperator", "Closed aggregate operator");
    }
    return Result<void>();
}

std::vector<std::string> AggregateOperator::getColumnNames() const {
    return columnNames_;
}

std::vector<DataType> AggregateOperator::getColumnTypes() const {
    return columnTypes_;
}

Result<void> AggregateOperator::buildGroups() {
    // 读取所有行并分组
    while (true) {
        auto nextResult = child_->next();
        if (!nextResult.isSuccess()) {
            return Result<void>(nextResult.getError());
        }

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) {
            break; // EOF
        }

        const Tuple& row = rowOpt->value();

        // 计算分组键
        auto keyResult = computeGroupKey(row);
        if (!keyResult.isSuccess()) {
            return Result<void>(keyResult.getError());
        }
        GroupKey groupKey = *keyResult.getValue();

        // 查找或创建分组
        auto it = groups_.find(groupKey);
        if (it == groups_.end()) {
            auto result = groups_.try_emplace(groupKey, aggregates_.size());
            it = result.first;
            groupOrder_.push_back(groupKey);
        }

        // 处理聚合函数
        RowContext context = buildRowContext(row);
        evaluator_.setRowContext(context);

        for (size_t i = 0; i < aggregates_.size(); ++i) {
            const auto& agg = aggregates_[i];
            auto evalResult = evaluator_.evaluate(agg.arg);
            if (!evalResult.isSuccess()) {
                return Result<void>(evalResult.getError());
            }

            const Value& val = *evalResult.getValue();
            it->second.states[i].addValue(agg.name, val);
        }
    }

    return Result<void>();
}

Result<AggregateOperator::GroupKey> AggregateOperator::computeGroupKey(const Tuple& row) {
    GroupKey key;

    for (const auto& expr : groupByExprs_) {
        RowContext context = buildRowContext(row);
        evaluator_.setRowContext(context);

        auto evalResult = evaluator_.evaluate(expr);
        if (!evalResult.isSuccess()) {
            return Result<GroupKey>(evalResult.getError());
        }

        key.keys.push_back(*evalResult.getValue());
    }

    return Result<GroupKey>(key);
}

RowContext AggregateOperator::buildRowContext(const Tuple& row) {
    RowContext context;

    auto columnNames = child_->getColumnNames();
    for (size_t i = 0; i < columnNames.size() && i < row.size(); ++i) {
        context[columnNames[i]] = row[i];
    }

    return context;
}

} // namespace executor
} // namespace minisql
