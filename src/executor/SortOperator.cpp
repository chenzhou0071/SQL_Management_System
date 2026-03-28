#include "SortOperator.h"
#include "../common/Logger.h"
#include <algorithm>

namespace minisql {
namespace executor {

SortOperator::SortOperator(OperatorPtr child, const std::vector<SortItem>& orderBy)
    : child_(child), orderBy_(orderBy), isOpen_(false), currentRowIndex_(0) {
    children_.push_back(child);
}

SortOperator::~SortOperator() {
    if (isOpen_) {
        close();
    }
}

Result<void> SortOperator::open() {
    if (isOpen_) {
        return Result<void>();
    }

    // 打开子算子
    auto openResult = child_->open();
    if (!openResult.isSuccess()) {
        return openResult;
    }

    // 物化并排序所有行
    auto sortResult = materializeAndSort();
    if (!sortResult.isSuccess()) {
        return sortResult;
    }

    // 获取列名和类型
    columnNames_ = child_->getColumnNames();
    columnTypes_ = child_->getColumnTypes();

    isOpen_ = true;
    LOG_INFO("SortOperator", "Opened sort operator with " +
            std::to_string(sortedRows_.size()) + " rows");
    return Result<void>();
}

Result<std::optional<Tuple>> SortOperator::next() {
    if (!isOpen_) {
        MiniSQLException error(ErrorCode::EXEC_TABLE_NOT_FOUND, "Operator not open");
        return Result<std::optional<Tuple>>(error);
    }

    if (currentRowIndex_ >= sortedRows_.size()) {
        return Result<std::optional<Tuple>>(std::nullopt); // EOF
    }

    Tuple row = sortedRows_[currentRowIndex_];
    currentRowIndex_++;
    return Result<std::optional<Tuple>>(row);
}

Result<void> SortOperator::close() {
    if (isOpen_) {
        child_->close();
        sortedRows_.clear();
        currentRowIndex_ = 0;
        isOpen_ = false;
        LOG_INFO("SortOperator", "Closed sort operator");
    }
    return Result<void>();
}

std::vector<std::string> SortOperator::getColumnNames() const {
    return columnNames_;
}

std::vector<DataType> SortOperator::getColumnTypes() const {
    return columnTypes_;
}

std::string SortOperator::getTableName() const {
    return child_->getTableName();
}

Result<void> SortOperator::materializeAndSort() {
    // 物化所有行
    while (true) {
        auto nextResult = child_->next();
        if (!nextResult.isSuccess()) {
            return Result<void>(nextResult.getError());
        }

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) {
            break; // EOF
        }

        sortedRows_.push_back(rowOpt->value());
    }

    // 排序
    std::sort(sortedRows_.begin(), sortedRows_.end(),
        [this](const Tuple& a, const Tuple& b) {
            return compareRows(a, b);
        });

    return Result<void>();
}

bool SortOperator::compareRows(const Tuple& left, const Tuple& right) {
    // 对每个排序项进行比较
    for (const auto& sortItem : orderBy_) {
        // 求值排序表达式
        RowContext leftContext = buildRowContext(left);
        RowContext rightContext = buildRowContext(right);

        evaluator_.setRowContext(leftContext);
        auto leftResult = evaluator_.evaluate(sortItem.expr);
        if (!leftResult.isSuccess()) {
            return false;
        }

        evaluator_.setRowContext(rightContext);
        auto rightResult = evaluator_.evaluate(sortItem.expr);
        if (!rightResult.isSuccess()) {
            return false;
        }

        Value leftVal = *leftResult.getValue();
        Value rightVal = *rightResult.getValue();

        // NULL 处理：NULL 值最小
        if (leftVal.isNull() && !rightVal.isNull()) {
            return true;  // NULL < any
        }
        if (!leftVal.isNull() && rightVal.isNull()) {
            return false; // any > NULL
        }
        if (leftVal.isNull() && rightVal.isNull()) {
            continue; // 相等，比较下一个排序项
        }

        // 比较值
        int cmp = leftVal.compareTo(rightVal);
        if (cmp != 0) {
            return sortItem.isAsc ? (cmp < 0) : (cmp > 0);
        }
        // 相等，继续比较下一个排序项
    }

    // 所有排序项都相等
    return false;
}

RowContext SortOperator::buildRowContext(const Tuple& row) {
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
