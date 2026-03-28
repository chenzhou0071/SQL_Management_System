#include "FilterOperator.h"
#include "../common/Logger.h"

namespace minisql {
namespace executor {

FilterOperator::FilterOperator(OperatorPtr child, parser::ExprPtr filterExpr)
    : child_(child), filterExpr_(filterExpr), isOpen_(false), currentDatabase_("default") {
    children_.push_back(child);
}

void FilterOperator::setCurrentDatabase(const std::string& dbName) {
    currentDatabase_ = dbName;
    evaluator_.setCurrentDatabase(dbName);
}

FilterOperator::~FilterOperator() {
    if (isOpen_) {
        close();
    }
}

Result<void> FilterOperator::open() {
    if (isOpen_) {
        return Result<void>();
    }

    // 打开子算子
    auto openResult = child_->open();
    if (!openResult.isSuccess()) {
        return openResult;
    }

    isOpen_ = true;
    LOG_INFO("FilterOperator", "Opened filter operator");
    return Result<void>();
}

Result<std::optional<Tuple>> FilterOperator::next() {
    if (!isOpen_) {
        MiniSQLException error(ErrorCode::EXEC_TABLE_NOT_FOUND, "Operator not open");
        return Result<std::optional<Tuple>>(error);
    }

    while (true) {
        // 从子算子获取下一行
        auto nextResult = child_->next();
        if (!nextResult.isSuccess()) {
            return nextResult;
        }

        auto rowOpt = nextResult.getValue();
        if (!rowOpt || !rowOpt->has_value()) {
            return Result<std::optional<Tuple>>(std::nullopt); // EOF
        }

        const Tuple& row = rowOpt->value();

        // 构建行上下文
        RowContext context = buildRowContext(row);
        evaluator_.setRowContext(context);

        // 求值过滤条件
        auto evalResult = evaluator_.evaluate(filterExpr_);
        if (!evalResult.isSuccess()) {
            return Result<std::optional<Tuple>>(evalResult.getError());
        }

        Value conditionResult = *evalResult.getValue();

        // 检查条件是否满足
        if (!conditionResult.isNull()) {
            bool shouldKeep = conditionResult.getBool();
            if (shouldKeep) {
                return Result<std::optional<Tuple>>(row);
            }
        }

        // 条件不满足，继续下一行
    }
}

Result<void> FilterOperator::close() {
    if (isOpen_) {
        child_->close();
        isOpen_ = false;
        LOG_INFO("FilterOperator", "Closed filter operator");
    }
    return Result<void>();
}

std::vector<std::string> FilterOperator::getColumnNames() const {
    return child_->getColumnNames();
}

std::vector<DataType> FilterOperator::getColumnTypes() const {
    return child_->getColumnTypes();
}

std::string FilterOperator::getTableName() const {
    return child_->getTableName();
}

RowContext FilterOperator::buildRowContext(const Tuple& row) {
    RowContext context;

    // 从子算��获取列名
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
}

} // namespace minisql
