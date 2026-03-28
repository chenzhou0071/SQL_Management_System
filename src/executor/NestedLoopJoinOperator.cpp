#include "NestedLoopJoinOperator.h"
#include "../common/Logger.h"

namespace minisql {
namespace executor {

NestedLoopJoinOperator::NestedLoopJoinOperator(OperatorPtr left, OperatorPtr right,
                                              parser::ExprPtr joinCond, JoinType joinType)
    : left_(left), right_(right), joinCond_(joinCond), joinType_(joinType),
      isOpen_(false), rightRowIndex_(0), leftRowMatched_(false),
      rightRowsIndex_(0), inRightJoinPhase2_(false) {
    children_.push_back(left);
    children_.push_back(right);
}

NestedLoopJoinOperator::~NestedLoopJoinOperator() {
    if (isOpen_) {
        close();
    }
}

Result<void> NestedLoopJoinOperator::open() {
    if (isOpen_) {
        return Result<void>();
    }

    // 打开左右子算子
    auto openLeft = left_->open();
    if (!openLeft.isSuccess()) {
        return openLeft;
    }

    auto openRight = right_->open();
    if (!openRight.isSuccess()) {
        return openRight;
    }

    // 加载右表所有行到内存
    auto loadResult = loadRightTable();
    if (!loadResult.isSuccess()) {
        return loadResult;
    }

    LOG_INFO("NestedLoopJoinOperator", "Loaded " + std::to_string(rightRows_.size()) + " rows from right table");

    // 初始化 RIGHT JOIN 的匹配标志数组
    if (joinType_ == JoinType::RIGHT) {
        rightRowsMatched_.resize(rightRows_.size(), false);
    }

    // 构建输出列名和类型（带表名前缀）
    std::string leftTableName = left_->getTableName();
    std::string rightTableName = right_->getTableName();

    auto leftNames = left_->getColumnNames();
    auto rightNames = right_->getColumnNames();

    // 左表列名：添加表名前缀
    for (const auto& name : leftNames) {
        if (!leftTableName.empty()) {
            columnNames_.push_back(leftTableName + "." + name);
        } else {
            columnNames_.push_back(name);
        }
    }

    // 右表列名：添加表名前缀
    for (const auto& name : rightNames) {
        if (!rightTableName.empty()) {
            columnNames_.push_back(rightTableName + "." + name);
        } else {
            columnNames_.push_back(name);
        }
    }

    auto leftTypes = left_->getColumnTypes();
    auto rightTypes = right_->getColumnTypes();
    columnTypes_.insert(columnTypes_.end(), leftTypes.begin(), leftTypes.end());
    columnTypes_.insert(columnTypes_.end(), rightTypes.begin(), rightTypes.end());

    isOpen_ = true;
    LOG_INFO("NestedLoopJoinOperator", "Opened nested loop join operator");
    return Result<void>();
}

Result<std::optional<Tuple>> NestedLoopJoinOperator::next() {
    if (!isOpen_) {
        MiniSQLException error(ErrorCode::EXEC_TABLE_NOT_FOUND, "Operator not open");
        return Result<std::optional<Tuple>>(error);
    }

    // RIGHT JOIN 第二阶段：输出未匹配的右行
    if (joinType_ == JoinType::RIGHT && inRightJoinPhase2_) {
        while (rightRowsIndex_ < rightRows_.size()) {
            if (!rightRowsMatched_[rightRowsIndex_]) {
                // 找到未匹配的右行，生成 NULL 填充的左行
                const Tuple& rightRow = rightRows_[rightRowsIndex_];
                Tuple nullLeftRow(left_->getColumnTypes().size());
                for (size_t i = 0; i < nullLeftRow.size(); ++i) {
                    nullLeftRow[i] = Value();  // NULL
                }

                Tuple joinedRow = joinRows(nullLeftRow, rightRow);
                rightRowsIndex_++;
                return Result<std::optional<Tuple>>(joinedRow);
            }
            rightRowsIndex_++;
        }
        return Result<std::optional<Tuple>>(std::nullopt); // EOF
    }

    // 第一阶段：正常的嵌套循环连接
    while (true) {
        // 如果没有当前左行，获取下一行
        if (!currentLeftRow_.has_value()) {
            auto nextLeft = left_->next();
            if (!nextLeft.isSuccess()) {
                return Result<std::optional<Tuple>>(nextLeft.getError());
            }

            auto leftRowOpt = nextLeft.getValue();
            if (!leftRowOpt->has_value()) {
                // 左表遍历完成
                if (joinType_ == JoinType::RIGHT) {
                    // 进入 RIGHT JOIN 第二阶段
                    inRightJoinPhase2_ = true;
                    rightRowsIndex_ = 0;
                    return next();  // 递归调用进入第二阶段
                }
                return Result<std::optional<Tuple>>(std::nullopt); // EOF
            }

            currentLeftRow_ = leftRowOpt->value();
            rightRowIndex_ = 0;
            leftRowMatched_ = false;  // 重置匹配标志
        }

        // 遍历右表行
        while (rightRowIndex_ < rightRows_.size()) {
            const Tuple& rightRow = rightRows_[rightRowIndex_];
            Tuple joinedRow = joinRows(currentLeftRow_.value(), rightRow);

            // 检查连接条件
            bool matched = false;
            if (joinCond_) {
                RowContext context = buildRowContext(currentLeftRow_.value(), rightRow);
                evaluator_.setRowContext(context);

                auto evalResult = evaluator_.evaluate(joinCond_);
                if (!evalResult.isSuccess()) {
                    return Result<std::optional<Tuple>>(evalResult.getError());
                }

                Value conditionResult = *evalResult.getValue();
                matched = !conditionResult.isNull() && conditionResult.getBool();
            } else {
                // 没有连接条件，笛卡尔积
                matched = true;
            }

            rightRowIndex_++;

            if (matched) {
                leftRowMatched_ = true;  // 标记左行匹配过
                if (joinType_ == JoinType::RIGHT) {
                    rightRowsMatched_[rightRowIndex_ - 1] = true;  // 标记右行被匹配
                }
                return Result<std::optional<Tuple>>(joinedRow);
            }
        }

        // 右表遍历完成，处理 LEFT JOIN 的未匹配情况
        if (joinType_ == JoinType::LEFT && !leftRowMatched_) {
            // 左外连接：如果没有匹配，生成 NULL 填充的右行
            Tuple nullRightRow(right_->getColumnTypes().size());
            for (size_t i = 0; i < nullRightRow.size(); ++i) {
                nullRightRow[i] = Value();  // NULL
            }

            Tuple joinedRow = joinRows(currentLeftRow_.value(), nullRightRow);
            currentLeftRow_.reset(); // 移动到下一左行
            leftRowMatched_ = false;

            return Result<std::optional<Tuple>>(joinedRow);
        }

        // 移动到下一左行
        currentLeftRow_.reset();
    }
}

Result<void> NestedLoopJoinOperator::close() {
    if (isOpen_) {
        left_->close();
        right_->close();
        rightRows_.clear();
        currentLeftRow_.reset();
        isOpen_ = false;
        LOG_INFO("NestedLoopJoinOperator", "Closed nested loop join operator");
    }
    return Result<void>();
}

std::vector<std::string> NestedLoopJoinOperator::getColumnNames() const {
    return columnNames_;
}

std::vector<DataType> NestedLoopJoinOperator::getColumnTypes() const {
    return columnTypes_;
}

std::string NestedLoopJoinOperator::getTableName() const {
    return left_->getTableName();
}

Result<void> NestedLoopJoinOperator::loadRightTable() {
    while (true) {
        auto nextResult = right_->next();
        if (!nextResult.isSuccess()) {
            return Result<void>(nextResult.getError());
        }

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) {
            break; // EOF
        }

        rightRows_.push_back(rowOpt->value());
    }

    return Result<void>();
}

Tuple NestedLoopJoinOperator::joinRows(const Tuple& left, const Tuple& right) {
    Tuple result;
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}

RowContext NestedLoopJoinOperator::buildRowContext(const Tuple& left, const Tuple& right) {
    RowContext context;

    // 获取左表和右表的名称
    std::string leftTableName = left_->getTableName();
    std::string rightTableName = right_->getTableName();

    // 添加左表列（带表名前缀和不带前缀两种形式）
    auto leftNames = left_->getColumnNames();
    for (size_t i = 0; i < leftNames.size() && i < left.size(); ++i) {
        // 不带表名前缀（用于向后兼容）
        context[leftNames[i]] = left[i];
        // 带表名前缀（用于 JOIN 条件）
        if (!leftTableName.empty()) {
            context[leftTableName + "." + leftNames[i]] = left[i];
        }
    }

    // 添加右表列（带表名前缀和不带前缀两种形式）
    auto rightNames = right_->getColumnNames();
    for (size_t i = 0; i < rightNames.size() && i < right.size(); ++i) {
        // 不带表名前缀
        context[rightNames[i]] = right[i];
        // 带表名前缀
        if (!rightTableName.empty()) {
            context[rightTableName + "." + rightNames[i]] = right[i];
        }
    }

    return context;
}

} // namespace executor
} // namespace minisql
