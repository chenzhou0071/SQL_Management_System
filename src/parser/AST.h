#pragma once

#include "Token.h"
#include "../common/Value.h"
#include <string>
#include <vector>
#include <memory>

namespace minisql {
namespace parser {

// 前向声明
class SelectStmt;

// ============================================================
// AST 节点类型枚举
// ============================================================
enum class ASTNodeType {
    UNKNOWN,

    // 语句节点
    SELECT_STMT,
    INSERT_STMT,
    UPDATE_STMT,
    DELETE_STMT,
    CREATE_STMT,
    CREATE_DATABASE_STMT,
    CREATE_INDEX_STMT,
    DROP_STMT,
    ALTER_STMT,
    USE_STMT,
    SHOW_STMT,
    DESCRIBE_STMT,
    EXPLAIN_STMT,
    ANALYZE_STMT,

    // 表达式节点
    EXPRESSION,
    BINARY_EXPR,
    UNARY_EXPR,
    LITERAL_EXPR,
    COLUMN_REF,
    FUNCTION_CALL,
    SUBQUERY_EXPR,       // 子查询表达式
    IN_SUBQUERY_EXPR,    // IN (SELECT ...) 表达式
    EXISTS_EXPR,         // EXISTS (SELECT ...) 表达式

    // 子句节点
    SELECT_CLAUSE,
    FROM_CLAUSE,
    WHERE_CLAUSE,
    GROUP_BY_CLAUSE,
    HAVING_CLAUSE,
    ORDER_BY_CLAUSE,
    LIMIT_CLAUSE,

    // 表引用
    TABLE_REF,
    JOIN_CLAUSE,

    // 列定义
    COLUMN_DEF,
    CONSTRAINT_DEF
};

// ============================================================
// AST 节点基类
// ============================================================
class ASTNode {
public:
    ASTNode(ASTNodeType type = ASTNodeType::UNKNOWN) : type_(type) {}
    virtual ~ASTNode() = default;

    ASTNodeType getType() const { return type_; }
    virtual std::string toString() const { return "ASTNode"; }

protected:
    ASTNodeType type_;
};

// ============================================================
// 表达式基类
// ============================================================
class Expression : public ASTNode {
public:
    Expression() : ASTNode(ASTNodeType::EXPRESSION) {}

    virtual std::string toString() const override { return "Expression"; }
};

using ExprPtr = std::shared_ptr<Expression>;

// ============================================================
// 字面量表达式
// ============================================================
class LiteralExpr : public Expression {
public:
    explicit LiteralExpr(const Value& value)
        : value_(value) {
        type_ = ASTNodeType::LITERAL_EXPR;
    }

    const Value& getValue() const { return value_; }
    std::string toString() const override {
        return value_.toString();
    }

private:
    Value value_;
};

// ============================================================
// 列引用表达式
// ============================================================
class ColumnRef : public Expression {
public:
    ColumnRef(const std::string& table, const std::string& column)
        : table_(table), column_(column) {
        type_ = ASTNodeType::COLUMN_REF;
    }

    const std::string& getTable() const { return table_; }
    const std::string& getColumn() const { return column_; }
    std::string toString() const override {
        if (table_.empty()) return column_;
        return table_ + "." + column_;
    }

private:
    std::string table_;
    std::string column_;
};

// ============================================================
// 二元表达式
// ============================================================
class BinaryExpr : public Expression {
public:
    BinaryExpr(ExprPtr left, const std::string& op, ExprPtr right)
        : left_(left), op_(op), right_(right) {
        type_ = ASTNodeType::BINARY_EXPR;
    }

    ExprPtr getLeft() const { return left_; }
    const std::string& getOp() const { return op_; }
    ExprPtr getRight() const { return right_; }
    std::string toString() const override {
        return "(" + left_->toString() + " " + op_ + " " + right_->toString() + ")";
    }

private:
    ExprPtr left_;
    std::string op_;
    ExprPtr right_;
};

// ============================================================
// 一元表达式
// ============================================================
class UnaryExpr : public Expression {
public:
    UnaryExpr(const std::string& op, ExprPtr operand)
        : op_(op), operand_(operand) {
        type_ = ASTNodeType::UNARY_EXPR;
    }

    const std::string& getOp() const { return op_; }
    ExprPtr getOperand() const { return operand_; }
    std::string toString() const override {
        return op_ + operand_->toString();
    }

private:
    std::string op_;
    ExprPtr operand_;
};

// ============================================================
// 函数调用表达式
// ============================================================
class FunctionCallExpr : public Expression {
public:
    FunctionCallExpr(const std::string& funcName, const std::vector<ExprPtr>& args)
        : funcName_(funcName), args_(args) {
        type_ = ASTNodeType::FUNCTION_CALL;
    }

    const std::string& getFuncName() const { return funcName_; }
    const std::vector<ExprPtr>& getArgs() const { return args_; }

    std::string toString() const override {
        std::string result = funcName_ + "(";
        for (size_t i = 0; i < args_.size(); ++i) {
            if (i > 0) result += ", ";
            result += args_[i]->toString();
        }
        result += ")";
        return result;
    }

private:
    std::string funcName_;
    std::vector<ExprPtr> args_;
};

// ============================================================
// 子查询表达式 (SELECT ...)
// ============================================================
class SubqueryExpr : public Expression {
public:
    SubqueryExpr(std::shared_ptr<SelectStmt> query)
        : query_(query) {
        type_ = ASTNodeType::SUBQUERY_EXPR;
    }

    std::shared_ptr<SelectStmt> getQuery() const { return query_; }

    std::string toString() const override {
        return "(SELECT ...)";
    }

private:
    std::shared_ptr<SelectStmt> query_;
};

// ============================================================
// IN 子查询表达式 (column IN (SELECT ...))
// ============================================================
class InSubqueryExpr : public Expression {
public:
    InSubqueryExpr(ExprPtr left, std::shared_ptr<SelectStmt> query, bool notIn = false)
        : left_(left), query_(query), notIn_(notIn) {
        type_ = ASTNodeType::IN_SUBQUERY_EXPR;
    }

    ExprPtr getLeft() const { return left_; }
    std::shared_ptr<SelectStmt> getQuery() const { return query_; }
    bool isNotIn() const { return notIn_; }

    std::string toString() const override {
        std::string op = notIn_ ? "NOT IN" : "IN";
        return left_->toString() + " " + op + " (SELECT ...)";
    }

private:
    ExprPtr left_;
    std::shared_ptr<SelectStmt> query_;
    bool notIn_;  // true for NOT IN, false for IN
};

// ============================================================
// EXISTS 子查询表达式 (EXISTS (SELECT ...))
// ============================================================
class ExistsExpr : public Expression {
public:
    ExistsExpr(std::shared_ptr<SelectStmt> query, bool notExists = false)
        : query_(query), notExists_(notExists) {
        type_ = ASTNodeType::EXISTS_EXPR;
    }

    std::shared_ptr<SelectStmt> getQuery() const { return query_; }
    bool isNotExists() const { return notExists_; }

    std::string toString() const override {
        std::string op = notExists_ ? "NOT EXISTS" : "EXISTS";
        return op + " (SELECT ...)";
    }

private:
    std::shared_ptr<SelectStmt> query_;
    bool notExists_;  // true for NOT EXISTS, false for EXISTS
};

// ============================================================
// 表引用
// ============================================================
struct TableRef : public ASTNode {
    std::string name;
    std::string alias;
    std::string database;
    std::shared_ptr<SelectStmt> subquery;  // FROM 子查询

    TableRef() : ASTNode(ASTNodeType::TABLE_REF) {}
    std::string toString() const override {
        if (subquery) {
            std::string subStr = "(subquery)";
            if (!alias.empty()) return subStr + " AS " + alias;
            return subStr;
        }
        if (!alias.empty()) return name + " AS " + alias;
        return name;
    }
};

// ============================================================
// JOIN 子句
// ============================================================
struct JoinClause : public ASTNode {
    std::string joinType;  // INNER, LEFT, RIGHT, FULL
    std::shared_ptr<TableRef> table;
    ExprPtr onCondition;

    JoinClause() : ASTNode(ASTNodeType::JOIN_CLAUSE) {}
};

// ============================================================
// ORDER BY 项
// ============================================================
struct OrderByItem {
    ExprPtr expr;
    bool ascending = true;
};

// ============================================================
// SELECT 语句
// ============================================================
class SelectStmt : public ASTNode {
public:
    SelectStmt() : ASTNode(ASTNodeType::SELECT_STMT), distinct(false), limit(-1), offset(-1) {}

    // SELECT 子句
    std::vector<ExprPtr> selectItems;
    bool distinct;

    // FROM 子句
    std::shared_ptr<TableRef> fromTable;
    std::vector<std::shared_ptr<JoinClause>> joins;

    // WHERE 子句
    ExprPtr whereClause;

    // GROUP BY 子句
    std::vector<ExprPtr> groupBy;

    // HAVING 子句
    ExprPtr havingClause;

    // ORDER BY 子句
    std::vector<OrderByItem> orderBy;

    // LIMIT 子句
    int limit;
    int offset;

    std::string toString() const override {
        return "SELECT ...";
    }
};

// ============================================================
// INSERT 语句
// ============================================================
class InsertStmt : public ASTNode {
public:
    InsertStmt() : ASTNode(ASTNodeType::INSERT_STMT) {}

    std::string table;
    std::vector<std::string> columns;
    std::vector<std::vector<ExprPtr>> valuesList;  // 支持批量插入

    std::string toString() const override {
        return "INSERT INTO " + table + " ...";
    }
};

// ============================================================
// UPDATE 语句
// ============================================================
class UpdateStmt : public ASTNode {
public:
    UpdateStmt() : ASTNode(ASTNodeType::UPDATE_STMT) {}

    std::string table;
    std::vector<std::pair<std::string, ExprPtr>> assignments;
    ExprPtr whereClause;

    std::string toString() const override {
        return "UPDATE " + table + " ...";
    }
};

// ============================================================
// DELETE 语句
// ============================================================
class DeleteStmt : public ASTNode {
public:
    DeleteStmt() : ASTNode(ASTNodeType::DELETE_STMT) {}

    std::string table;
    ExprPtr whereClause;

    std::string toString() const override {
        return "DELETE FROM " + table + " ...";
    }
};

// ============================================================
// CREATE TABLE 语句
// ============================================================
struct ColumnDefNode : public ASTNode {
    std::string name;
    DataType type;
    int length = 0;
    bool notNull = false;
    bool primaryKey = false;
    bool unique = false;
    bool autoIncrement = false;
    std::string defaultValue;

    ColumnDefNode() : ASTNode(ASTNodeType::COLUMN_DEF) {}
};

struct ConstraintDefNode : public ASTNode {
    ConstraintType type;
    std::string name;
    std::vector<std::string> columns;

    ConstraintDefNode() : ASTNode(ASTNodeType::CONSTRAINT_DEF) {}
};

// 表级索引定义
struct IndexDefNode : public ASTNode {
    std::string name;
    std::vector<std::string> columns;
    std::string type = "BTREE";
    bool unique = false;

    IndexDefNode() : ASTNode(ASTNodeType::UNKNOWN) {}
};

class CreateTableStmt : public ASTNode {
public:
    CreateTableStmt() : ASTNode(ASTNodeType::CREATE_STMT), ifNotExists(false) {}

    std::string table;
    std::vector<std::shared_ptr<ColumnDefNode>> columns;
    std::vector<std::shared_ptr<ConstraintDefNode>> constraints;
    std::vector<IndexDef> indexes;  // 表级索引定义
    bool ifNotExists;
    std::string engine;
    std::string comment;
    std::vector<ForeignKeyDef> foreignKeys;  // 外键定义

    std::string toString() const override {
        return "CREATE TABLE " + table + " ...";
    }
};

// ============================================================
// CREATE DATABASE 语句
// ============================================================
class CreateDatabaseStmt : public ASTNode {
public:
    CreateDatabaseStmt() : ASTNode(ASTNodeType::CREATE_DATABASE_STMT), ifNotExists(false) {}

    std::string database;
    bool ifNotExists;

    std::string toString() const override {
        return "CREATE DATABASE " + database;
    }
};

// ============================================================
// CREATE INDEX 语句
// ============================================================
class CreateIndexStmt : public ASTNode {
public:
    CreateIndexStmt() : ASTNode(ASTNodeType::CREATE_INDEX_STMT), unique(false), ifNotExists(false) {}

    std::string indexName;
    std::string tableName;
    std::vector<std::string> columnNames;  // 索引列（支持复合索引）
    bool unique;
    bool ifNotExists;

    std::string toString() const override {
        return "CREATE " + (unique ? std::string("UNIQUE ") : "") + "INDEX " + indexName + " ON " + tableName;
    }
};

// ============================================================
// DROP 语句
// ============================================================
class DropStmt : public ASTNode {
public:
    DropStmt() : ASTNode(ASTNodeType::DROP_STMT), ifExists(false) {}

    std::string objectType;  // TABLE, DATABASE, INDEX, VIEW
    std::string name;
    bool ifExists;

    std::string toString() const override {
        return "DROP " + objectType + " " + name;
    }
};

// ============================================================
// ALTER TABLE 语句
// ============================================================

// ALTER 操作类型
enum class AlterOperationType {
    ADD_COLUMN,
    DROP_COLUMN,
    MODIFY_COLUMN,
    RENAME_COLUMN,
    ADD_CONSTRAINT,
    DROP_CONSTRAINT,
    RENAME_TABLE
};

// 列修改操作
struct AlterModifyColumn {
    std::string oldName;
    std::string newName;
    DataType newType;
    int newLength = 0;
    bool notNull = false;
};

// ALTER TABLE 语句
class AlterTableStmt : public ASTNode {
public:
    AlterTableStmt() : ASTNode(ASTNodeType::ALTER_STMT) {}

    std::string tableName;
    AlterOperationType operation;

    // 用于 ADD/DROP/MODIFY COLUMN
    std::shared_ptr<ColumnDefNode> columnDef;

    // 用于 RENAME COLUMN
    std::string oldColumnName;
    std::string newColumnName;

    // 用于 RENAME TABLE
    std::string newTableName;

    // 用于 ADD/DROP CONSTRAINT
    std::shared_ptr<ConstraintDefNode> constraint;

    std::string toString() const override {
        return "ALTER TABLE " + tableName + " ...";
    }
};

// ============================================================
// USE 语句
// ============================================================
class UseStmt : public ASTNode {
public:
    UseStmt() : ASTNode(ASTNodeType::USE_STMT) {}

    std::string database;

    std::string toString() const override {
        return "USE " + database;
    }
};

// ============================================================
// SHOW 语句
// ============================================================
class ShowStmt : public ASTNode {
public:
    ShowStmt() : ASTNode(ASTNodeType::SHOW_STMT) {}

    std::string objectType;  // DATABASES, TABLES, COLUMNS, INDEX
    std::string database;
    std::string table;

    std::string toString() const override {
        return "SHOW " + objectType;
    }
};

// ============================================================
// DESCRIBE 语句
// ============================================================
class DescribeStmt : public ASTNode {
public:
    DescribeStmt() : ASTNode(ASTNodeType::DESCRIBE_STMT) {}

    std::string tableName;

    std::string toString() const override {
        return "DESCRIBE " + tableName;
    }
};

// ============================================================
// EXPLAIN 语句
// ============================================================
class ExplainStmt : public ASTNode {
public:
    ExplainStmt() : ASTNode(ASTNodeType::EXPLAIN_STMT) {}

    std::shared_ptr<ASTNode> innerStatement;  // 被解释的内部语句
    bool formatJSON = false;                 // 是否使用 JSON 格式

    std::string toString() const override {
        return "EXPLAIN ...";
    }
};

// ============================================================
// ANALYZE TABLE 语句
// ============================================================
class AnalyzeStmt : public ASTNode {
public:
    AnalyzeStmt() : ASTNode(ASTNodeType::ANALYZE_STMT) {}

    std::string tableName;  // 要分析的表名

    std::string toString() const override {
        return "ANALYZE TABLE " + tableName;
    }
};

}  // namespace parser
}  // namespace minisql
