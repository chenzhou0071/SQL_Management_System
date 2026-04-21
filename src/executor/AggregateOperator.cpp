#include "AggregateOperator.h"
#include "../common/Logger.h"

namespace minisql {
namespace executor {

// ============================================================
// 聚合算子 - 实现GROUP BY + 聚合函数 + HAVING
// ============================================================
// 聚合函数支持：
// - COUNT: 计数（包括NULL值）
// - SUM: 求和（忽略NULL值）
// - AVG: 平均值（忽略NULL值）
// - MIN: 最小值（忽略NULL值）
// - MAX: 最大值（忽略NULL值）
//
// GROUP BY处理流程：
// 1. 读取子算子的所有行
// 2. 根据GROUP BY表达式计算分组键
// 3. 将行分组到对应的组
// 4. 对每个组计算聚合函数值
// 5. 应用HAVING条件过滤分组
// 6. 输出满足HAVING条件的分组结果
//
// 实现策略：
// - 使用哈希表（groups_）存储分组状态
// - GroupKey: 分组键（GROUP BY列的值组合）
// - AggregateState: 每个组的聚合函数状态（count/sum/min/max）
// - 物化模式：一次性读取所有数据，在内存中分组聚合
//
// 注意事项：
// - NULL值处理：COUNT统计NULL，其他函数忽略NULL
// - 没有GROUP BY时：所有行作为一个组
// - HAVING在分组后过滤，WHERE在分组前过滤
// ============================================================

// ============================================================
// 单聚合函数状态 - 添加值到聚合状态
// ============================================================
// 不同聚合函数的处理：
// - COUNT: 简单计数（包括NULL值）
// - SUM/AVG: 累加数值，忽略NULL（AVG还需计数）
// - MIN/MAX: 维护最小/最大值，忽略NULL
// ============================================================

void AggregateOperator::SingleAggregateState::addValue(const std::string& funcName, const Value& val) {
    if (funcName == "COUNT") {
        count++;  // COUNT计数所有行，包括NULL值
    } else if (val.isNull()) {
        return; // NULL值不参与其他聚合计算（SUM/AVG/MIN/MAX）
    } else if (funcName == "SUM") {
        // SUM累加数值
        if (!sum.has_value()) {
            sum = 0.0;  // 初始化为0
        }
        if (val.getType() == DataType::INT || val.getType() == DataType::BIGINT) {
            *sum += val.getInt();  // 整数转浮点累加
        } else if (val.getType() == DataType::DOUBLE) {
            *sum += val.getDouble();  // 浮点直接累加
        }
    } else if (funcName == "AVG") {
        // AVG需要同时跟踪count和sum（最后用sum/count计算）
        count++;  // 记录非NULL值的数量
        if (!sum.has_value()) {
            sum = 0.0;
        }
        if (val.getType() == DataType::INT || val.getType() == DataType::BIGINT) {
            *sum += val.getInt();
        } else if (val.getType() == DataType::DOUBLE) {
            *sum += val.getDouble();
        }
    } else if (funcName == "MIN") {
        // MIN维护最小值
        if (!min.has_value()) {
            min = val;  // 第一个值
        } else {
            if (val.compareTo(*min) < 0) {
                min = val;  // 更新最小值
            }
        }
    } else if (funcName == "MAX") {
        // MAX维护最大值
        if (!max.has_value()) {
            max = val;  // 第一个值
        } else {
            if (val.compareTo(*max) > 0) {
                max = val;  // 更新最大值
            }
        }
    }
}

// ============================================================
// 计算聚合函数最终结果
// ============================================================
// 各聚合函数的最终计算：
// - COUNT: 返回count值
// - SUM: 返回sum值，如果无值则返回NULL
// - AVG: 返回sum/count，如果无值或count=0则返回NULL
// - MIN/MAX: 返回min/max值，如果无值则返回NULL
// ============================================================
Value AggregateOperator::SingleAggregateState::compute(const std::string& funcName) {
    if (funcName == "COUNT") {
        return Value(static_cast<int64_t>(count));  // COUNT返回整数
    } else if (funcName == "SUM") {
        if (!sum.has_value()) {
            return Value(); // NULL - 无非NULL值
        }
        return Value(*sum);  // 返回累加总和
    } else if (funcName == "AVG") {
        if (!sum.has_value() || count == 0) {
            return Value(); // NULL - 无非NULL值
        }
        return Value(*sum / static_cast<double>(count));  // 平均值
    } else if (funcName == "MIN") {
        if (!min.has_value()) {
            return Value(); // NULL
        }
        return *min;  // 最小值
    } else if (funcName == "MAX") {
        if (!max.has_value()) {
            return Value(); // NULL
        }
        return *max;  // 最大值
    }
    return Value(); // Unknown function - 返回NULL
}

AggregateOperator::AggregateOperator(OperatorPtr child,
                                   const std::vector<parser::ExprPtr>& groupByExprs,
                                   const std::vector<AggregateFunc>& aggregates,
                                   parser::ExprPtr havingClause)
    : child_(child), groupByExprs_(groupByExprs), aggregates_(aggregates),
      havingClause_(havingClause), isOpen_(false), currentGroupIndex_(0) {
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

// ============================================================
// 获取下一行聚合结果（应用HAVING过滤）
// ============================================================
// 输出流程：
// 1. 遍历所有分组（按groupOrder_顺序）
// 2. 对每个分组：
//    a) 构建HAVING求值的行上下文（GROUP BY列 + 聚合列）
//    b) 求值HAVING条件
//    c) 如果HAVING为真，输出该分组结果
//    d) 如果HAVING为假，跳过该分组
// 3. 返回nullopt表示所有分组已输出
//
// HAVING vs WHERE：
// - WHERE在聚合前过滤原始行
// - HAVING在聚合后过滤分组结果
// - HAVING可以使用聚合函数结果（WHERE不能）
// ============================================================
Result<std::optional<Tuple>> AggregateOperator::next() {
    if (!isOpen_) {
        MiniSQLException error(ErrorCode::EXEC_TABLE_NOT_FOUND, "Operator not open");
        return Result<std::optional<Tuple>>(error);
    }

    // 遍历分组，跳过不满足HAVING条件的分组
    while (currentGroupIndex_ < groupOrder_.size()) {
        const GroupKey& groupKey = groupOrder_[currentGroupIndex_];
        auto it = groups_.find(groupKey);
        if (it == groups_.end()) {
            currentGroupIndex_++;
            continue;
        }
        AggregateState& state = it->second;

        // 构建HAVING求值的行上下文：GROUP BY列 + 聚合列
        Tuple havingRow;
        for (const auto& val : groupKey.keys) {
            havingRow.push_back(val);  // GROUP BY列的值
        }
        for (size_t i = 0; i < aggregates_.size(); ++i) {
            havingRow.push_back(state.states[i].compute(aggregates_[i].name));  // 聚合结果
        }

        // 检查HAVING条件
        if (havingClause_) {
            RowContext context = buildRowContext(havingRow);  // 构建行上下文
            evaluator_.setRowContext(context);
            auto havingResult = evaluator_.evaluate(havingClause_);
            if (!havingResult.isSuccess()) {
                return Result<std::optional<Tuple>>(havingResult.getError());
            }
            Value havingVal = *havingResult.getValue();
            // HAVING条件为NULL或false，跳过该分组
            if (havingVal.isNull() || !havingVal.getBool()) {
                currentGroupIndex_++;
                continue;
            }
        }

        // 构建输出行：GROUP BY列 + 聚合结果
        Tuple row;
        for (const auto& val : groupKey.keys) {
            row.push_back(val);  // GROUP BY列
        }
        for (size_t i = 0; i < aggregates_.size(); ++i) {
            row.push_back(state.states[i].compute(aggregates_[i].name));  // 聚合列
        }

        currentGroupIndex_++;  // 移动到下一分组
        return Result<std::optional<Tuple>>(row);
    }

    return Result<std::optional<Tuple>>(std::nullopt); // EOF - 所有分组已输出
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

std::string AggregateOperator::getTableName() const {
    return child_->getTableName();
}

// ============================================================
// 构建分组并计算聚合（核心算法）
// ============================================================
// 流程：
// 1. 读取子算子的所有行
// 2. 对每一行：
//    a) 计算GROUP BY表达式的值作为分组键
//    b) 查找或创建对应的分组状态
//    c) 对每个聚合函数计算参数值并添加到状态
// 3. groups_映射：GroupKey -> AggregateState
// 4. groupOrder_列表：记录分组顺序（用于有序输出）
//
// 注意：整个聚合过程在open()阶段一次性完成
//       next()只需遍历groups_输出结果
// ============================================================
Result<void> AggregateOperator::buildGroups() {
    // 读取所有行并分组聚合
    while (true) {
        auto nextResult = child_->next();
        if (!nextResult.isSuccess()) {
            return Result<void>(nextResult.getError());
        }

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) {
            break; // EOF - 子算子数据读取完成
        }

        const Tuple& row = rowOpt->value();

        // 计算分组键（GROUP BY表达式的值）
        auto keyResult = computeGroupKey(row);
        if (!keyResult.isSuccess()) {
            return Result<void>(keyResult.getError());
        }
        GroupKey groupKey = *keyResult.getValue();

        // 查找或创建分组状态
        auto it = groups_.find(groupKey);
        if (it == groups_.end()) {
            // 新分组：创建空聚合状态并加入groups_
            auto result = groups_.try_emplace(groupKey, aggregates_.size());
            it = result.first;  // 新创建的分组迭代器
            groupOrder_.push_back(groupKey);  // 记录分组顺序
        }

        // 处理聚合函数：对每个聚合函数计算参数并更新状态
        RowContext context = buildRowContext(row);  // 构建行上下文
        evaluator_.setRowContext(context);

        for (size_t i = 0; i < aggregates_.size(); ++i) {
            const auto& agg = aggregates_[i];
            // 计算聚合函数的参数表达式（如SUM(amount)中的amount）
            auto evalResult = evaluator_.evaluate(agg.arg);
            if (!evalResult.isSuccess()) {
                return Result<void>(evalResult.getError());
            }

            Value val = *evalResult.getValue();
            // 将参数值添加到聚合状态
            it->second.states[i].addValue(agg.name, val);
        }
    }

    return Result<void>();
}

// ============================================================
// 计算分组键（辅助函数）
// ============================================================
// 分组键：GROUP BY表达式值的组合
// 例如：GROUP BY name, age -> GroupKey包含name和age的值
// 使用哈希表存储分组，GroupKey需要支持哈希和比较
// ============================================================
Result<AggregateOperator::GroupKey> AggregateOperator::computeGroupKey(const Tuple& row) {
    GroupKey key;

    for (const auto& expr : groupByExprs_) {
        // 构建行上下文用于求值GROUP BY表达式
        RowContext context = buildRowContext(row);
        evaluator_.setRowContext(context);

        // 求值GROUP BY表达式
        auto evalResult = evaluator_.evaluate(expr);
        if (!evalResult.isSuccess()) {
            return Result<GroupKey>(evalResult.getError());
        }

        key.keys.push_back(*evalResult.getValue());  // 添加值到分组键
    }

    return Result<GroupKey>(key);
}

RowContext AggregateOperator::buildRowContext(const Tuple& row) {
    RowContext context;

    // 从子算子获取列名
    auto columnNames = child_->getColumnNames();

    // 构建列名到值的映射
    for (size_t i = 0; i < columnNames.size() && i < row.size(); ++i) {
        const std::string& fullColumnName = columnNames[i];

        // 添加完整列名（可能包含表名前缀，如 "users.name"）
        context[fullColumnName] = row[i];

        // 如果列名包含表名前缀，也注册不带前缀的列名（用于向后兼容）
        size_t dotPos = fullColumnName.find('.');
        if (dotPos != std::string::npos) {
            std::string shortName = fullColumnName.substr(dotPos + 1);
            // 只有当没有冲突时才添加短名称
            if (context.find(shortName) == context.end()) {
                context[shortName] = row[i];
            }
        }
    }

    return context;
}

} // namespace executor
} // namespace minisql
