#include "SubqueryScanOperator.h"
#include "../common/Logger.h"

namespace minisql {
namespace executor {

SubqueryScanOperator::SubqueryScanOperator(OperatorPtr subqueryOp, const std::string& alias)
    : subqueryOp_(subqueryOp), alias_(alias), isOpen_(false), currentRowIndex_(0) {
    children_.push_back(subqueryOp);
}

SubqueryScanOperator::~SubqueryScanOperator() {
    if (isOpen_) {
        close();
    }
}

Result<void> SubqueryScanOperator::open() {
    if (isOpen_) {
        return Result<void>();
    }

    // 打开子查询算子
    auto openResult = subqueryOp_->open();
    if (!openResult.isSuccess()) {
        return openResult;
    }

    // 物化子查询的所有结果到内存
    while (true) {
        auto nextResult = subqueryOp_->next();
        if (!nextResult.isSuccess()) {
            return Result<void>(nextResult.getError());
        }

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) {
            break; // EOF
        }

        materializedRows_.push_back(rowOpt->value());
    }

    // 关闭子查询算子
    subqueryOp_->close();

    // 构建列名和类型
    auto subNames = subqueryOp_->getColumnNames();
    auto subTypes = subqueryOp_->getColumnTypes();

    // 如果有别名，添加别名前缀
    if (!alias_.empty()) {
        for (const auto& name : subNames) {
            columnNames_.push_back(alias_ + "." + name);
        }
    } else {
        columnNames_ = subNames;
    }

    columnTypes_ = subTypes;

    currentRowIndex_ = 0;
    isOpen_ = true;

    LOG_INFO("SubqueryScanOperator", "Opened subquery scan: " + alias_ + " with " + std::to_string(materializedRows_.size()) + " rows");
    return Result<void>();
}

Result<std::optional<Tuple>> SubqueryScanOperator::next() {
    if (!isOpen_) {
        MiniSQLException error(ErrorCode::EXEC_TABLE_NOT_FOUND, "Operator not open");
        return Result<std::optional<Tuple>>(error);
    }

    if (currentRowIndex_ >= materializedRows_.size()) {
        return Result<std::optional<Tuple>>(std::nullopt); // EOF
    }

    Tuple row = materializedRows_[currentRowIndex_];
    currentRowIndex_++;
    return Result<std::optional<Tuple>>(row);
}

Result<void> SubqueryScanOperator::close() {
    if (isOpen_) {
        materializedRows_.clear();
        currentRowIndex_ = 0;
        isOpen_ = false;
        LOG_INFO("SubqueryScanOperator", "Closed subquery scan: " + alias_);
    }
    return Result<void>();
}

std::vector<std::string> SubqueryScanOperator::getColumnNames() const {
    return columnNames_;
}

std::vector<DataType> SubqueryScanOperator::getColumnTypes() const {
    return columnTypes_;
}

std::string SubqueryScanOperator::getTableName() const {
    return alias_;
}

} // namespace executor
} // namespace minisql
