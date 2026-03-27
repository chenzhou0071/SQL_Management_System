#include "LimitOperator.h"
#include "../common/Logger.h"

namespace minisql {
namespace executor {

LimitOperator::LimitOperator(OperatorPtr child, size_t limit, size_t offset)
    : child_(child), limit_(limit), offset_(offset),
      isOpen_(false), skipCount_(0), produceCount_(0) {
    children_.push_back(child);
}

LimitOperator::~LimitOperator() {
    if (isOpen_) {
        close();
    }
}

Result<void> LimitOperator::open() {
    if (isOpen_) {
        return Result<void>();
    }

    // 打开子算子
    auto openResult = child_->open();
    if (!openResult.isSuccess()) {
        return openResult;
    }

    skipCount_ = 0;
    produceCount_ = 0;
    isOpen_ = true;

    LOG_INFO("LimitOperator", "Opened limit operator: LIMIT " +
            std::to_string(limit_) + " OFFSET " + std::to_string(offset_));
    return Result<void>();
}

Result<std::optional<Tuple>> LimitOperator::next() {
    if (!isOpen_) {
        MiniSQLException error(ErrorCode::EXEC_TABLE_NOT_FOUND, "Operator not open");
        return Result<std::optional<Tuple>>(error);
    }

    // 跳过 OFFSET 指定的行数
    while (skipCount_ < offset_) {
        auto nextResult = child_->next();
        if (!nextResult.isSuccess()) {
            return nextResult;
        }

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) {
            return Result<std::optional<Tuple>>(std::nullopt); // EOF
        }

        skipCount_++;
    }

    // 检查是否已达到 LIMIT
    if (produceCount_ >= limit_) {
        return Result<std::optional<Tuple>>(std::nullopt); // LIMIT reached
    }

    // 获取下一行
    auto nextResult = child_->next();
    if (!nextResult.isSuccess()) {
        return nextResult;
    }

    auto rowOpt = nextResult.getValue();
    if (!rowOpt->has_value()) {
        return Result<std::optional<Tuple>>(std::nullopt); // EOF
    }

    produceCount_++;
    return Result<std::optional<Tuple>>(rowOpt->value());
}

Result<void> LimitOperator::close() {
    if (isOpen_) {
        child_->close();
        isOpen_ = false;
        LOG_INFO("LimitOperator", "Closed limit operator");
    }
    return Result<void>();
}

std::vector<std::string> LimitOperator::getColumnNames() const {
    return child_->getColumnNames();
}

std::vector<DataType> LimitOperator::getColumnTypes() const {
    return child_->getColumnTypes();
}

} // namespace executor
} // namespace minisql
