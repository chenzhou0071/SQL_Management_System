#include "ProjectOperator.h"
#include "../common/Logger.h"

namespace minisql {
namespace executor {

ProjectOperator::ProjectOperator(OperatorPtr child, const std::vector<parser::ExprPtr>& projections)
    : child_(child), projections_(projections), isOpen_(false) {
    children_.push_back(child);
}

ProjectOperator::~ProjectOperator() {
    if (isOpen_) {
        close();
    }
}

Result<void> ProjectOperator::open() {
    if (isOpen_) {
        return Result<void>();
    }

    // 打开子算子
    auto openResult = child_->open();
    if (!openResult.isSuccess()) {
        return openResult;
    }

    // 缓存列名和类型（不需要读取第一行）
    for (const auto& proj : projections_) {
        columnNames_.push_back(deriveColumnName(proj));
        columnTypes_.push_back(deriveExpressionType(proj));
    }

    isOpen_ = true;
    LOG_INFO("ProjectOperator", "Opened project operator");
    return Result<void>();
}

Result<std::optional<Tuple>> ProjectOperator::next() {
    if (!isOpen_) {
        MiniSQLException error(ErrorCode::EXEC_TABLE_NOT_FOUND, "Operator not open");
        return Result<std::optional<Tuple>>(error);
    }

    // 从子算子获取下一行
    auto nextResult = child_->next();
    if (!nextResult.isSuccess()) {
        return nextResult;
    }

    auto rowOpt = nextResult.getValue();
    if (!rowOpt->has_value()) {
        return Result<std::optional<Tuple>>(std::nullopt); // EOF
    }

    const Tuple& inputRow = rowOpt->value();

    // 构建行上下文
    RowContext context = buildRowContext(inputRow);
    evaluator_.setRowContext(context);

    // 计算每个投影表达式
    Tuple outputRow;
    for (const auto& proj : projections_) {
        auto evalResult = evaluator_.evaluate(proj);
        if (!evalResult.isSuccess()) {
            return Result<std::optional<Tuple>>(evalResult.getError());
        }
        outputRow.push_back(*evalResult.getValue());
    }

    return Result<std::optional<Tuple>>(outputRow);
}

Result<void> ProjectOperator::close() {
    if (isOpen_) {
        child_->close();
        isOpen_ = false;
        LOG_INFO("ProjectOperator", "Closed project operator");
    }
    return Result<void>();
}

std::vector<std::string> ProjectOperator::getColumnNames() const {
    return columnNames_;
}

std::vector<DataType> ProjectOperator::getColumnTypes() const {
    return columnTypes_;
}

RowContext ProjectOperator::buildRowContext(const Tuple& row) {
    RowContext context;

    // 从子算子获取列名
    auto columnNames = child_->getColumnNames();

    // 构建列名到值的映射
    for (size_t i = 0; i < columnNames.size() && i < row.size(); ++i) {
        context[columnNames[i]] = row[i];
    }

    return context;
}

std::string ProjectOperator::deriveColumnName(parser::ExprPtr expr) {
    if (!expr) return "?";

    switch (expr->getType()) {
        case parser::ASTNodeType::COLUMN_REF: {
            auto colRef = static_cast<parser::ColumnRef*>(expr.get());
            return colRef->getColumn();
        }

        case parser::ASTNodeType::LITERAL_EXPR: {
            return "literal";
        }

        case parser::ASTNodeType::FUNCTION_CALL: {
            auto funcCall = static_cast<parser::FunctionCallExpr*>(expr.get());
            return funcCall->getFuncName() + "(...)";
        }

        case parser::ASTNodeType::BINARY_EXPR: {
            auto binary = static_cast<parser::BinaryExpr*>(expr.get());
            return deriveColumnName(binary->getLeft()) + " " +
                   binary->getOp() + " " +
                   deriveColumnName(binary->getRight());
        }

        default:
            return "expr";
    }
}

DataType ProjectOperator::deriveExpressionType(parser::ExprPtr expr) {
    if (!expr) return DataType::UNKNOWN;

    switch (expr->getType()) {
        case parser::ASTNodeType::LITERAL_EXPR: {
            auto literal = static_cast<parser::LiteralExpr*>(expr.get());
            return literal->getValue().getType();
        }

        case parser::ASTNodeType::COLUMN_REF: {
            // 从子算子获取列类型
            auto colRef = static_cast<parser::ColumnRef*>(expr.get());
            auto childTypes = child_->getColumnTypes();
            auto childNames = child_->getColumnNames();

            // 查找列索引
            for (size_t i = 0; i < childNames.size(); ++i) {
                if (childNames[i] == colRef->getColumn()) {
                    return childTypes[i];
                }
            }
            return DataType::UNKNOWN;
        }

        case parser::ASTNodeType::BINARY_EXPR: {
            auto binary = static_cast<parser::BinaryExpr*>(expr.get());
            const std::string& op = binary->getOp();

            // 比较运算返回布尔值
            if (op == "=" || op == "!=" || op == "<" || op == ">" ||
                op == "<=" || op == ">=") {
                return DataType::BOOLEAN;
            }

            // 逻辑运算返回布尔值
            if (op == "AND" || op == "OR") {
                return DataType::BOOLEAN;
            }

            // 算术运算：如果任一操作数是 DOUBLE，结果为 DOUBLE
            DataType leftType = deriveExpressionType(binary->getLeft());
            DataType rightType = deriveExpressionType(binary->getRight());

            if (leftType == DataType::DOUBLE || rightType == DataType::DOUBLE) {
                return DataType::DOUBLE;
            }
            return DataType::INT;
        }

        case parser::ASTNodeType::FUNCTION_CALL: {
            auto funcCall = static_cast<parser::FunctionCallExpr*>(expr.get());
            const std::string& funcName = funcCall->getFuncName();

            // 聚合函数类型推导需要更复杂的逻辑
            if (funcName == "COUNT") return DataType::BIGINT;
            if (funcName == "SUM") return DataType::DOUBLE;
            if (funcName == "AVG") return DataType::DOUBLE;
            if (funcName == "MAX" || funcName == "MIN") {
                // 返回参数类型
                if (!funcCall->getArgs().empty()) {
                    return deriveExpressionType(funcCall->getArgs()[0]);
                }
                return DataType::UNKNOWN;
            }

            // 标量函数
            if (funcName == "UPPER" || funcName == "LOWER" ||
                funcName == "CONCAT" || funcName == "LENGTH") {
                return DataType::VARCHAR;
            }

            return DataType::UNKNOWN;
        }

        default:
            return DataType::UNKNOWN;
    }
}

} // namespace executor
} // namespace minisql
