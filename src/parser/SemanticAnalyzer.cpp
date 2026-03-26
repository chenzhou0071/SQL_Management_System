#include "SemanticAnalyzer.h"
#include "../common/Error.h"

namespace minisql {
namespace parser {

SemanticAnalyzer::SemanticAnalyzer()
    : tableManager_(storage::TableManager::getInstance()) {
}

void SemanticAnalyzer::analyze(std::shared_ptr<ASTNode> stmt) {
    switch (stmt->getType()) {
        case ASTNodeType::SELECT_STMT:
            analyzeSelect(std::dynamic_pointer_cast<SelectStmt>(stmt));
            break;
        case ASTNodeType::INSERT_STMT:
            analyzeInsert(std::dynamic_pointer_cast<InsertStmt>(stmt));
            break;
        case ASTNodeType::UPDATE_STMT:
            analyzeUpdate(std::dynamic_pointer_cast<UpdateStmt>(stmt));
            break;
        case ASTNodeType::DELETE_STMT:
            analyzeDelete(std::dynamic_pointer_cast<DeleteStmt>(stmt));
            break;
        case ASTNodeType::CREATE_STMT:
            analyzeCreateTable(std::dynamic_pointer_cast<CreateTableStmt>(stmt));
            break;
        case ASTNodeType::DROP_STMT:
            analyzeDrop(std::dynamic_pointer_cast<DropStmt>(stmt));
            break;
        case ASTNodeType::ALTER_STMT:
            analyzeAlterTable(std::dynamic_pointer_cast<AlterTableStmt>(stmt));
            break;
        case ASTNodeType::USE_STMT:
            analyzeUse(std::dynamic_pointer_cast<UseStmt>(stmt));
            break;
        case ASTNodeType::SHOW_STMT:
            analyzeShow(std::dynamic_pointer_cast<ShowStmt>(stmt));
            break;
        default:
            break;
    }
}

std::string SemanticAnalyzer::getCurrentDatabase() {
    // TODO: 从 Session/Connection 获取当前数据库
    // 目前返回空字符串，使用默认行为
    return "";
}

void SemanticAnalyzer::checkTableExists(const std::string& table) {
    std::string db = getCurrentDatabase();
    if (!db.empty()) {
        auto result = tableManager_.tableExists(db, table);
        if (result.isError() || !*result.getValue()) {
            throw SemanticError("Table '" + table + "' does not exist");
        }
    }
}

void SemanticAnalyzer::checkColumnExists(const std::string& table, const std::string& column) {
    std::string db = getCurrentDatabase();
    if (!db.empty()) {
        auto result = tableManager_.getTableDef(db, table);
        if (result.isError()) {
            throw SemanticError("Table '" + table + "' does not exist");
        }

        TableDef tableDef = *result.getValue();
        bool found = false;
        for (const auto& col : tableDef.columns) {
            if (col.name == column) {
                found = true;
                break;
            }
        }

        if (!found) {
            throw SemanticError("Column '" + column + "' does not exist in table '" + table + "'");
        }
    }
}

void SemanticAnalyzer::checkDataType(DataType actual, DataType expected) {
    if (actual != expected) {
        throw SemanticError("Type mismatch: expected " +
            TypeUtils::typeToString(expected) + " but got " +
            TypeUtils::typeToString(actual));
    }
}

void SemanticAnalyzer::validateExpression(ExprPtr expr, const std::string& contextTable) {
    if (!expr) return;

    switch (expr->getType()) {
        case ASTNodeType::COLUMN_REF: {
            auto colRef = std::dynamic_pointer_cast<ColumnRef>(expr);
            if (!colRef->getTable().empty()) {
                checkColumnExists(colRef->getTable(), colRef->getColumn());
            } else if (!contextTable.empty()) {
                checkColumnExists(contextTable, colRef->getColumn());
            }
            break;
        }
        case ASTNodeType::BINARY_EXPR: {
            auto binary = std::dynamic_pointer_cast<BinaryExpr>(expr);
            validateExpression(binary->getLeft(), contextTable);
            validateExpression(binary->getRight(), contextTable);
            break;
        }
        case ASTNodeType::UNARY_EXPR: {
            auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr);
            validateExpression(unary->getOperand(), contextTable);
            break;
        }
        case ASTNodeType::FUNCTION_CALL: {
            auto func = std::dynamic_pointer_cast<FunctionCallExpr>(expr);
            // 验证函数参数
            for (const auto& arg : func->getArgs()) {
                validateExpression(arg, contextTable);
            }
            break;
        }
        default:
            break;
    }
}

void SemanticAnalyzer::analyzeSelect(std::shared_ptr<SelectStmt> stmt) {
    // 检查 FROM 子句中的表
    if (stmt->fromTable) {
        checkTableExists(stmt->fromTable->name);
    }

    // 检查 SELECT 列表中的表达式
    for (const auto& expr : stmt->selectItems) {
        validateExpression(expr, stmt->fromTable ? stmt->fromTable->name : "");
    }

    // 检查 WHERE 子句
    if (stmt->whereClause) {
        validateExpression(stmt->whereClause, stmt->fromTable ? stmt->fromTable->name : "");
    }

    // 检查 JOIN 条件
    for (const auto& join : stmt->joins) {
        if (join->table) {
            checkTableExists(join->table->name);
        }
        if (join->onCondition) {
            validateExpression(join->onCondition, stmt->fromTable ? stmt->fromTable->name : "");
        }
    }
}

void SemanticAnalyzer::analyzeInsert(std::shared_ptr<InsertStmt> stmt) {
    checkTableExists(stmt->table);
}

void SemanticAnalyzer::analyzeUpdate(std::shared_ptr<UpdateStmt> stmt) {
    checkTableExists(stmt->table);

    // 检查 SET 子句中的列
    for (const auto& assignment : stmt->assignments) {
        checkColumnExists(stmt->table, assignment.first);
    }

    // 检查 WHERE 子句
    if (stmt->whereClause) {
        validateExpression(stmt->whereClause, stmt->table);
    }
}

void SemanticAnalyzer::analyzeDelete(std::shared_ptr<DeleteStmt> stmt) {
    checkTableExists(stmt->table);

    // 检查 WHERE 子句
    if (stmt->whereClause) {
        validateExpression(stmt->whereClause, stmt->table);
    }
}

void SemanticAnalyzer::analyzeCreateTable(std::shared_ptr<CreateTableStmt> stmt) {
    // 检查表是否已存在（除非使用 IF NOT EXISTS）
    std::string db = getCurrentDatabase();
    if (!db.empty() && !stmt->ifNotExists) {
        auto result = tableManager_.tableExists(db, stmt->table);
        if (result.isSuccess() && *result.getValue()) {
            throw SemanticError("Table '" + stmt->table + "' already exists");
        }
    }

    // 验证列定义
    for (const auto& colDef : stmt->columns) {
        if (colDef->name.empty()) {
            throw SemanticError("Column name cannot be empty");
        }

        // 检查是否有重复列名
        for (size_t i = 0; i < stmt->columns.size(); ++i) {
            if (i != stmt->columns.size() - 1 && stmt->columns[i]->name == colDef->name) {
                throw SemanticError("Duplicate column name: " + colDef->name);
            }
        }
    }
}

void SemanticAnalyzer::analyzeDrop(std::shared_ptr<DropStmt> stmt) {
    if (stmt->objectType == "TABLE") {
        std::string db = getCurrentDatabase();
        if (!db.empty() && !stmt->ifExists) {
            auto result = tableManager_.tableExists(db, stmt->name);
            if (result.isSuccess() && !*result.getValue()) {
                throw SemanticError("Table '" + stmt->name + "' does not exist");
            }
        }
    }
}

void SemanticAnalyzer::analyzeAlterTable(std::shared_ptr<AlterTableStmt> stmt) {
    // 检查表是否存在
    checkTableExists(stmt->tableName);

    switch (stmt->operation) {
        case AlterOperationType::ADD_COLUMN:
            if (stmt->columnDef && stmt->columnDef->name.empty()) {
                throw SemanticError("Column name cannot be empty");
            }
            break;
        case AlterOperationType::DROP_COLUMN:
            if (stmt->oldColumnName.empty()) {
                throw SemanticError("Column name cannot be empty");
            }
            // 检查列是否存在
            checkColumnExists(stmt->tableName, stmt->oldColumnName);
            break;
        case AlterOperationType::MODIFY_COLUMN:
        case AlterOperationType::RENAME_COLUMN:
            if (stmt->columnDef && stmt->columnDef->name.empty() && !stmt->oldColumnName.empty()) {
                // RENAME COLUMN 情况
                checkColumnExists(stmt->tableName, stmt->oldColumnName);
            } else if (stmt->columnDef) {
                checkColumnExists(stmt->tableName, stmt->columnDef->name);
            }
            break;
        case AlterOperationType::RENAME_TABLE:
            if (stmt->newTableName.empty()) {
                throw SemanticError("New table name cannot be empty");
            }
            break;
        case AlterOperationType::ADD_CONSTRAINT:
        case AlterOperationType::DROP_CONSTRAINT:
            // 约束检查（简化实现）
            break;
    }
}

void SemanticAnalyzer::analyzeUse(std::shared_ptr<UseStmt> stmt) {
    // 检查数据库是否存在
    if (!tableManager_.databaseExists(stmt->database)) {
        throw SemanticError("Database '" + stmt->database + "' does not exist");
    }
}

void SemanticAnalyzer::analyzeShow(std::shared_ptr<ShowStmt> stmt) {
    if (stmt->objectType == "DATABASES") {
        // SHOW DATABASES 不需要特殊检查
    } else if (stmt->objectType == "TABLES") {
        // 需要当前数据库存在
        std::string db = getCurrentDatabase();
        if (db.empty()) {
            throw SemanticError("No database selected");
        }
    }
}

}  // namespace parser
}  // namespace minisql
