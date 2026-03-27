#include "NestedLoopJoinOperator.h"
#include "../common/Logger.h"

namespace minisql {
namespace executor {

NestedLoopJoinOperator::NestedLoopJoinOperator(OperatorPtr left, OperatorPtr right,
                                              parser::ExprPtr joinCond, JoinType joinType)
    : left_(left), right_(right), joinCond_(joinCond), joinType_(joinType),
      isOpen_(false), rightRowIndex_(0) {
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

    // 构建输出列名和类型
    auto leftNames = left_->getColumnNames();
    auto rightNames = right_->getColumnNames();
    columnNames_.insert(columnNames_.end(), leftNames.begin(), leftNames.end());
    columnNames_.insert(columnNames_.end(), rightNames.begin(), rightNames.end());

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

    while (true) {
        // 如果没有当前左行，获取下一行
        if (!currentLeftRow_.has_value()) {
            auto nextLeft = left_->next();
            if (!nextLeft.isSuccess()) {
                return Result<std::optional<Tuple>>(nextLeft.getError());
            }

            auto leftRowOpt = nextLeft.getValue();
            if (!leftRowOpt->has_value()) {
                return Result<std::optional<Tuple>>(std::nullopt); // EOF
            }

            currentLeftRow_ = leftRowOpt->value();
            rightRowIndex_ = 0;
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

                const Value& conditionResult = *evalResult.getValue();
                if (!conditionResult.isNull() && conditionResult.getBool()) {
                    matched = true;
                }
            } else {
                // 没有连接条件，笛卡尔积
                matched = true;
            }

            rightRowIndex_++;

            if (matched) {
                return Result<std::optional<Tuple>>(joinedRow);
            }
        }

        // 右表遍历完成，处理 LEFT JOIN 的未匹配情况
        if (joinType_ == JoinType::LEFT && rightRowIndex_ >= rightRows_.size()) {
            // 左外连接：如果没有匹配，生成 NULL 填充的右行
            bool hasMatch = false;
            // 重新检查是否有匹配（这里简化处理）
            // 实际实现需要记录是否匹配过

            // 生成 NULL 填充的行
            Tuple nullRightRow(right_->getColumnTypes().size());
            for (size_t i = 0; i < nullRightRow.size(); ++i) {
                nullRightRow[i] = Value();
            }

            Tuple joinedRow = joinRows(currentLeftRow_.value(), nullRightRow);
            currentLeftRow_.reset(); // 移动到下一左行

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

    // 添加左表列（不带表名前缀，直接使用列名）
    auto leftNames = left_->getColumnNames();
    for (size_t i = 0; i < leftNames.size() && i < left.size(); ++i) {
        context[leftNames[i]] = left[i];
    }

    // 添加右表列（覆盖同名的左表列）
    auto rightNames = right_->getColumnNames();
    for (size_t i = 0; i < rightNames.size() && i < right.size(); ++i) {
        context[rightNames[i]] = right[i];
    }

    return context;
}

} // namespace executor
} // namespace minisql
