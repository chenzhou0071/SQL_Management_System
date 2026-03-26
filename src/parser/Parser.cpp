#include "Parser.h"
#include "../common/Error.h"

namespace minisql {
namespace parser {

Parser::Parser(Lexer& lexer)
    : lexer_(lexer), current_(TokenType::UNKNOWN, "", 0, 0) {
    advance();  // 预读第一个 token
}

bool Parser::checkKeyword(const std::string& keyword) const {
    return current_.type == TokenType::KEYWORD && current_.value == keyword;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::matchKeyword(const std::string& keyword) {
    if (checkKeyword(keyword)) {
        advance();
        return true;
    }
    return false;
}

void Parser::expect(TokenType type, const std::string& message) {
    if (!check(type)) {
        throw MiniSQLException(
            ErrorCode::PARSER_MISSING_TOKEN,
            message + ", got: " + current_.toString()
        );
    }
    advance();
}

void Parser::expectKeyword(const std::string& keyword, const std::string& message) {
    if (!checkKeyword(keyword)) {
        throw MiniSQLException(
            ErrorCode::PARSER_MISSING_TOKEN,
            message + ", got: " + current_.toString()
        );
    }
    advance();
}

std::shared_ptr<ASTNode> Parser::parseStatement() {
    if (current_.type == TokenType::EOF_TOKEN) {
        throw MiniSQLException(ErrorCode::PARSER_SYNTAX_ERROR, "Unexpected end of input");
    }

    if (checkKeyword("USE")) return parseUseStatement();
    if (checkKeyword("SHOW")) return parseShowStatement();
    if (checkKeyword("SELECT")) return parseSelectStatement();
    if (checkKeyword("INSERT")) return parseInsertStatement();
    if (checkKeyword("UPDATE")) return parseUpdateStatement();
    if (checkKeyword("DELETE")) return parseDeleteStatement();
    if (checkKeyword("CREATE")) return parseCreateStatement();
    if (checkKeyword("DROP")) return parseDropStatement();
    if (checkKeyword("ALTER")) return parseAlterStatement();

    throw MiniSQLException(
        ErrorCode::PARSER_SYNTAX_ERROR,
        "Unknown statement: " + current_.value
    );
}

std::shared_ptr<ASTNode> Parser::parseUseStatement() {
    expectKeyword("USE", "Expected USE");

    if (!check(TokenType::IDENTIFIER)) {
        throw MiniSQLException(
            ErrorCode::PARSER_MISSING_TOKEN,
            "Expected database name after USE"
        );
    }

    auto stmt = std::make_shared<UseStmt>();
    stmt->database = current_.value;
    advance();

    match(TokenType::SEMICOLON);  // 可选的分号

    return stmt;
}

std::shared_ptr<ASTNode> Parser::parseShowStatement() {
    expectKeyword("SHOW", "Expected SHOW");

    auto stmt = std::make_shared<ShowStmt>();

    if (checkKeyword("DATABASES")) {
        stmt->objectType = "DATABASES";
        advance();
    } else if (checkKeyword("TABLES")) {
        stmt->objectType = "TABLES";
        advance();
    } else {
        throw MiniSQLException(
            ErrorCode::PARSER_SYNTAX_ERROR,
            "Expected DATABASES or TABLES after SHOW"
        );
    }

    match(TokenType::SEMICOLON);
    return stmt;
}

// SELECT 语句解析（简化版本）
std::shared_ptr<SelectStmt> Parser::parseSelectStatement() {
    auto stmt = std::make_shared<SelectStmt>();

    expectKeyword("SELECT", "Expected SELECT");

    // DISTINCT
    if (checkKeyword("DISTINCT")) {
        stmt->distinct = true;
        advance();
    }

    // SELECT 列表
    if (check(TokenType::OPERATOR) && current_.value == "*") {
        advance();
        // * 可以表示为空列表或特殊的标记
    } else {
        while (!checkKeyword("FROM") && !check(TokenType::EOF_TOKEN)) {
            ExprPtr expr = parseExpression();
            stmt->selectItems.push_back(expr);

            if (!match(TokenType::COMMA)) {
                break;
            }
        }
    }

    // FROM 子句
    if (matchKeyword("FROM")) {
        if (check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD)) {
            auto tableRef = std::make_shared<TableRef>();
            tableRef->name = current_.value;
            advance();

            // 检查别名
            if (check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD)) {
                // 检查是否是 AS 关键字
                if (!checkKeyword("AS")) {
                    tableRef->alias = current_.value;
                    advance();
                } else {
                    advance();  // 跳过 AS
                    if (check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD)) {
                        tableRef->alias = current_.value;
                        advance();
                    }
                }
            }

            stmt->fromTable = tableRef;
        }

        // JOIN 子句
        while (checkKeyword("JOIN") || checkKeyword("INNER") ||
               checkKeyword("LEFT") || checkKeyword("RIGHT") ||
               checkKeyword("FULL") || checkKeyword("CROSS")) {
            auto join = std::make_shared<JoinClause>();

            // JOIN 类型
            if (checkKeyword("INNER")) {
                join->joinType = "INNER";
                advance();
                expectKeyword("JOIN", "Expected JOIN after INNER");
            } else if (checkKeyword("LEFT")) {
                join->joinType = "LEFT";
                advance();
                if (checkKeyword("OUTER")) {
                    advance();  // 跳过 OUTER
                }
                expectKeyword("JOIN", "Expected JOIN after LEFT");
            } else if (checkKeyword("RIGHT")) {
                join->joinType = "RIGHT";
                advance();
                if (checkKeyword("OUTER")) {
                    advance();  // 跳过 OUTER
                }
                expectKeyword("JOIN", "Expected JOIN after RIGHT");
            } else if (checkKeyword("FULL")) {
                join->joinType = "FULL";
                advance();
                if (checkKeyword("OUTER")) {
                    advance();  // 跳过 OUTER
                }
                expectKeyword("JOIN", "Expected JOIN after FULL");
            } else if (checkKeyword("CROSS")) {
                join->joinType = "CROSS";
                advance();
                expectKeyword("JOIN", "Expected JOIN after CROSS");
            } else {
                join->joinType = "INNER";
                advance();  // 跳过 JOIN
            }

            // 表引用
            join->table = std::make_shared<TableRef>();
            if (check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD)) {
                join->table->name = current_.value;
                advance();

                // 检查别名
                if (check(TokenType::IDENTIFIER) || checkKeyword("AS")) {
                    if (checkKeyword("AS")) {
                        advance();
                    }
                    if (check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD)) {
                        join->table->alias = current_.value;
                        advance();
                    }
                }
            }

            // ON 条件
            if (matchKeyword("ON")) {
                join->onCondition = parseExpression();
            }

            stmt->joins.push_back(join);
        }
    }

    // WHERE 子句（简化）
    if (matchKeyword("WHERE")) {
        stmt->whereClause = parseExpression();
    }

    match(TokenType::SEMICOLON);

    return stmt;
}

std::shared_ptr<InsertStmt> Parser::parseInsertStatement() {
    auto stmt = std::make_shared<InsertStmt>();

    expectKeyword("INSERT", "Expected INSERT");
    expectKeyword("INTO", "Expected INTO");

    // 表名
    if (check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD)) {
        stmt->table = current_.value;
        advance();
    }

    // 列名列表
    if (check(TokenType::LEFT_PAREN)) {
        advance();
        while (!check(TokenType::RIGHT_PAREN) && !check(TokenType::EOF_TOKEN)) {
            if (check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD)) {
                stmt->columns.push_back(current_.value);
                advance();
            }
            if (!match(TokenType::COMMA)) {
                break;
            }
        }
        expect(TokenType::RIGHT_PAREN, "Expected ')' after column list");
    }

    expectKeyword("VALUES", "Expected VALUES");

    // 值列表
    while (check(TokenType::LEFT_PAREN)) {
        advance();
        std::vector<ExprPtr> values;

        while (!check(TokenType::RIGHT_PAREN) && !check(TokenType::EOF_TOKEN)) {
            values.push_back(parseExpression());
            if (!match(TokenType::COMMA)) {
                break;
            }
        }

        expect(TokenType::RIGHT_PAREN, "Expected ')' after values");
        stmt->valuesList.push_back(values);

        if (!match(TokenType::COMMA)) {
            break;
        }
    }

    match(TokenType::SEMICOLON);
    return stmt;
}

std::shared_ptr<UpdateStmt> Parser::parseUpdateStatement() {
    auto stmt = std::make_shared<UpdateStmt>();

    expectKeyword("UPDATE", "Expected UPDATE");

    // 表名
    if (check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD)) {
        stmt->table = current_.value;
        advance();
    }

    expectKeyword("SET", "Expected SET");

    // 赋值列表
    while (!checkKeyword("WHERE") && !check(TokenType::EOF_TOKEN)) {
        if (check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD)) {
            std::string column = current_.value;
            advance();

            expect(TokenType::OPERATOR, "Expected '=' after column name");

            ExprPtr value = parseExpression();
            stmt->assignments.push_back({column, value});
        }

        if (!match(TokenType::COMMA)) {
            break;
        }
    }

    // WHERE 子句
    if (matchKeyword("WHERE")) {
        stmt->whereClause = parseExpression();
    }

    match(TokenType::SEMICOLON);
    return stmt;
}

std::shared_ptr<DeleteStmt> Parser::parseDeleteStatement() {
    auto stmt = std::make_shared<DeleteStmt>();

    expectKeyword("DELETE", "Expected DELETE");
    expectKeyword("FROM", "Expected FROM");

    // 表名
    if (check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD)) {
        stmt->table = current_.value;
        advance();
    }

    // WHERE 子句
    if (matchKeyword("WHERE")) {
        stmt->whereClause = parseExpression();
    }

    match(TokenType::SEMICOLON);
    return stmt;
}

std::shared_ptr<CreateTableStmt> Parser::parseCreateStatement() {
    auto stmt = std::make_shared<CreateTableStmt>();

    expectKeyword("CREATE", "Expected CREATE");

    // 检查是否有 IF NOT EXISTS
    if (checkKeyword("IF")) {
        advance();
        if (checkKeyword("NOT")) {
            advance();
            if (checkKeyword("EXISTS")) {
                stmt->ifNotExists = true;
                advance();
            }
        }
    }

    // 对象类型
    if (checkKeyword("TABLE")) {
        advance();
    } else {
        throw MiniSQLException(ErrorCode::PARSER_SYNTAX_ERROR, "Expected TABLE after CREATE");
    }

    // 表名
    if (check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD)) {
        stmt->table = current_.value;
        advance();
    }

    // 列定义
    if (check(TokenType::LEFT_PAREN)) {
        advance();

        while (!check(TokenType::RIGHT_PAREN) && !check(TokenType::EOF_TOKEN)) {
            auto columnDef = std::make_shared<ColumnDefNode>();

            // 列名
            if (check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD)) {
                columnDef->name = current_.value;
                advance();
            }

            // 数据类型
            if (check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD)) {
                std::string typeName = current_.value;
                advance();

                // 转换为 DataType（简化处理）
                if (typeName == "INT" || typeName == "INTEGER") {
                    columnDef->type = DataType::INT;
                } else if (typeName == "VARCHAR" || typeName == "CHAR") {
                    columnDef->type = DataType::VARCHAR;
                    // 检查长度
                    if (check(TokenType::LEFT_PAREN)) {
                        advance();
                        if (check(TokenType::INT_LITERAL)) {
                            columnDef->length = std::stoi(current_.value);
                            advance();
                        }
                        expect(TokenType::RIGHT_PAREN, "Expected ')' after type length");
                    }
                } else if (typeName == "FLOAT" || typeName == "DOUBLE") {
                    columnDef->type = DataType::FLOAT;
                } else {
                    columnDef->type = DataType::UNKNOWN;
                }
            }

            // 约束（简化处理）
            while (!check(TokenType::COMMA) && !check(TokenType::RIGHT_PAREN) &&
                   !check(TokenType::EOF_TOKEN)) {
                if (checkKeyword("NOT")) {
                    advance();
                    if (checkKeyword("NULL")) {
                        columnDef->notNull = true;
                        advance();
                    }
                } else if (checkKeyword("PRIMARY")) {
                    advance();
                    if (checkKeyword("KEY")) {
                        columnDef->primaryKey = true;
                        advance();
                    }
                } else if (checkKeyword("UNIQUE")) {
                    columnDef->unique = true;
                    advance();
                } else if (checkKeyword("AUTO_INCREMENT")) {
                    columnDef->autoIncrement = true;
                    advance();
                } else {
                    // 跳过未识别的标记
                    advance();
                }
            }

            stmt->columns.push_back(columnDef);

            if (!match(TokenType::COMMA)) {
                break;
            }
        }

        expect(TokenType::RIGHT_PAREN, "Expected ')' after column definitions");
    }

    match(TokenType::SEMICOLON);
    return stmt;
}

std::shared_ptr<AlterTableStmt> Parser::parseAlterStatement() {
    auto stmt = std::make_shared<AlterTableStmt>();

    expectKeyword("ALTER", "Expected ALTER");
    expectKeyword("TABLE", "Expected TABLE after ALTER");

    // 表名
    if (!check(TokenType::IDENTIFIER)) {
        throw MiniSQLException(ErrorCode::PARSER_MISSING_TOKEN, "Expected table name after ALTER TABLE");
    }
    stmt->tableName = current_.value;
    advance();

    // ALTER 操作类型
    if (checkKeyword("ADD")) {
        advance();
        // ADD COLUMN 或 ADD CONSTRAINT
        if (checkKeyword("COLUMN")) {
            advance();
            stmt->operation = AlterOperationType::ADD_COLUMN;
            stmt->columnDef = std::make_shared<ColumnDefNode>();
            stmt->columnDef->name = current_.value;
            advance();
            // 解析列类型
            if (checkKeyword("INT") || checkKeyword("INTEGER")) {
                stmt->columnDef->type = DataType::INT;
                advance();
            } else if (checkKeyword("VARCHAR")) {
                stmt->columnDef->type = DataType::VARCHAR;
                advance();
                if (check(TokenType::LEFT_PAREN)) {
                    advance();
                    if (check(TokenType::INT_LITERAL)) {
                        stmt->columnDef->length = std::stoi(current_.value);
                        advance();
                    }
                    expect(TokenType::RIGHT_PAREN, "Expected ')' after length");
                }
            } else if (checkKeyword("DOUBLE") || checkKeyword("FLOAT")) {
                stmt->columnDef->type = DataType::DOUBLE;
                advance();
            } else if (checkKeyword("DATE")) {
                stmt->columnDef->type = DataType::DATE;
                advance();
            }
            // 检查列约束
            while (checkKeyword("NOT") || checkKeyword("PRIMARY") || checkKeyword("KEY") ||
                   checkKeyword("UNIQUE") || checkKeyword("AUTO_INCREMENT")) {
                if (checkKeyword("NOT")) {
                    advance();
                    expectKeyword("NULL", "Expected NULL after NOT");
                    stmt->columnDef->notNull = true;
                } else if (checkKeyword("PRIMARY") || checkKeyword("KEY")) {
                    if (checkKeyword("PRIMARY")) advance();
                    expectKeyword("KEY", "Expected KEY after PRIMARY");
                    stmt->columnDef->primaryKey = true;
                } else if (checkKeyword("UNIQUE")) {
                    advance();
                    stmt->columnDef->unique = true;
                } else if (checkKeyword("AUTO_INCREMENT")) {
                    advance();
                    stmt->columnDef->autoIncrement = true;
                }
            }
        } else if (checkKeyword("CONSTRAINT")) {
            advance();
            stmt->operation = AlterOperationType::ADD_CONSTRAINT;
            stmt->constraint = std::make_shared<ConstraintDefNode>();
            // 可选约束名
            if (check(TokenType::IDENTIFIER)) {
                stmt->constraint->name = current_.value;
                advance();
            }
            // 约束类型
            if (checkKeyword("PRIMARY") || checkKeyword("KEY")) {
                if (checkKeyword("PRIMARY")) advance();
                expectKeyword("KEY", "Expected KEY");
                stmt->constraint->type = ConstraintType::PRIMARY_KEY;
                // 列名
                if (check(TokenType::LEFT_PAREN)) {
                    advance();
                    stmt->constraint->columns = parseIdentifierList();
                    expect(TokenType::RIGHT_PAREN, "Expected ')'");
                }
            } else if (checkKeyword("UNIQUE")) {
                advance();
                stmt->constraint->type = ConstraintType::UNIQUE;
                if (check(TokenType::LEFT_PAREN)) {
                    advance();
                    stmt->constraint->columns = parseIdentifierList();
                    expect(TokenType::RIGHT_PAREN, "Expected ')'");
                }
            } else if (checkKeyword("FOREIGN") || checkKeyword("REFERENCES")) {
                // 外键约束简化处理
                if (checkKeyword("FOREIGN")) advance();
                expectKeyword("REFERENCES", "Expected REFERENCES");
                stmt->constraint->type = ConstraintType::FOREIGN_KEY;
            }
        } else {
            // 默认为 ADD COLUMN（列名）
            stmt->operation = AlterOperationType::ADD_COLUMN;
            stmt->columnDef = std::make_shared<ColumnDefNode>();
            stmt->columnDef->name = current_.value;
            advance();
            // 解析列类型
            if (checkKeyword("INT") || checkKeyword("INTEGER")) {
                stmt->columnDef->type = DataType::INT;
                advance();
            } else if (checkKeyword("VARCHAR")) {
                stmt->columnDef->type = DataType::VARCHAR;
                advance();
            } else if (checkKeyword("DOUBLE") || checkKeyword("FLOAT")) {
                stmt->columnDef->type = DataType::DOUBLE;
                advance();
            }
        }
    } else if (checkKeyword("DROP")) {
        advance();
        if (checkKeyword("COLUMN")) {
            advance();
            stmt->operation = AlterOperationType::DROP_COLUMN;
            stmt->oldColumnName = current_.value;
            advance();
        } else if (checkKeyword("CONSTRAINT")) {
            advance();
            stmt->operation = AlterOperationType::DROP_CONSTRAINT;
            stmt->constraint = std::make_shared<ConstraintDefNode>();
            stmt->constraint->name = current_.value;
            advance();
        } else {
            // 默认为 DROP COLUMN
            stmt->operation = AlterOperationType::DROP_COLUMN;
            stmt->oldColumnName = current_.value;
            advance();
        }
    } else if (checkKeyword("MODIFY") || checkKeyword("CHANGE")) {
        // MODIFY 或 CHANGE column
        bool isChange = checkKeyword("CHANGE");
        advance();
        if (checkKeyword("COLUMN")) advance();
        stmt->operation = AlterOperationType::MODIFY_COLUMN;
        stmt->columnDef = std::make_shared<ColumnDefNode>();

        if (isChange) {
            // CHANGE old_name new_name type ...
            stmt->columnDef->name = current_.value;
            advance();
            stmt->oldColumnName = stmt->columnDef->name;
            stmt->newColumnName = current_.value;
            advance();
        } else {
            stmt->columnDef->name = current_.value;
            advance();
        }

        // 解析新类型
        if (checkKeyword("INT") || checkKeyword("INTEGER")) {
            stmt->columnDef->type = DataType::INT;
            advance();
        } else if (checkKeyword("VARCHAR")) {
            stmt->columnDef->type = DataType::VARCHAR;
            advance();
            if (check(TokenType::LEFT_PAREN)) {
                advance();
                if (check(TokenType::INT_LITERAL)) {
                    stmt->columnDef->length = std::stoi(current_.value);
                    advance();
                }
                expect(TokenType::RIGHT_PAREN, "Expected ')'");
            }
        } else if (checkKeyword("DOUBLE") || checkKeyword("FLOAT")) {
            stmt->columnDef->type = DataType::DOUBLE;
            advance();
        }
        // 检查 NOT NULL
        if (checkKeyword("NOT")) {
            advance();
            expectKeyword("NULL", "Expected NULL after NOT");
            stmt->columnDef->notNull = true;
        }
    } else if (checkKeyword("RENAME")) {
        advance();
        if (checkKeyword("TO")) {
            advance();
            stmt->operation = AlterOperationType::RENAME_TABLE;
            stmt->newTableName = current_.value;
            advance();
        } else if (checkKeyword("COLUMN")) {
            advance();
            stmt->operation = AlterOperationType::RENAME_COLUMN;
            stmt->oldColumnName = current_.value;
            advance();
            expectKeyword("TO", "Expected TO");
            stmt->newColumnName = current_.value;
            advance();
        }
    }

    match(TokenType::SEMICOLON);
    return stmt;
}

std::shared_ptr<DropStmt> Parser::parseDropStatement() {
    auto stmt = std::make_shared<DropStmt>();

    expectKeyword("DROP", "Expected DROP");

    // 检查 IF EXISTS
    if (checkKeyword("IF")) {
        advance();
        if (checkKeyword("EXISTS")) {
            stmt->ifExists = true;
            advance();
        }
    }

    // 对象类型
    if (checkKeyword("TABLE")) {
        stmt->objectType = "TABLE";
        advance();
    } else if (checkKeyword("DATABASE")) {
        stmt->objectType = "DATABASE";
        advance();
    } else {
        throw MiniSQLException(ErrorCode::PARSER_SYNTAX_ERROR, "Expected TABLE or DATABASE after DROP");
    }

    // 对象名
    if (check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD)) {
        stmt->name = current_.value;
        advance();
    }

    match(TokenType::SEMICOLON);
    return stmt;
}

// 表达式解析
ExprPtr Parser::parseExpression() {
    return parseOrExpression();
}

ExprPtr Parser::parseOrExpression() {
    ExprPtr left = parseAndExpression();

    while (checkKeyword("OR")) {
        std::string op = current_.value;
        advance();
        ExprPtr right = parseAndExpression();
        left = std::make_shared<BinaryExpr>(left, op, right);
    }

    return left;
}

ExprPtr Parser::parseAndExpression() {
    ExprPtr left = parseNotExpression();

    while (checkKeyword("AND")) {
        std::string op = current_.value;
        advance();
        ExprPtr right = parseNotExpression();
        left = std::make_shared<BinaryExpr>(left, op, right);
    }

    return left;
}

ExprPtr Parser::parseNotExpression() {
    if (checkKeyword("NOT")) {
        std::string op = current_.value;
        advance();
        ExprPtr operand = parseNotExpression();
        return std::make_shared<UnaryExpr>(op, operand);
    }

    return parseComparisonExpression();
}

ExprPtr Parser::parseComparisonExpression() {
    ExprPtr left = parseAdditiveExpression();

    while (check(TokenType::OPERATOR)) {
        std::string op = current_.value;
        // 检查是否是比较运算符
        if (op == "=" || op == "<>" || op == "!=" || op == "<" ||
            op == ">" || op == "<=" || op == ">=") {
            advance();
            ExprPtr right = parseAdditiveExpression();
            left = std::make_shared<BinaryExpr>(left, op, right);
        } else {
            break;
        }
    }

    return left;
}

ExprPtr Parser::parseAdditiveExpression() {
    ExprPtr left = parseMultiplicativeExpression();

    while (check(TokenType::OPERATOR)) {
        std::string op = current_.value;
        if (op == "+" || op == "-") {
            advance();
            ExprPtr right = parseMultiplicativeExpression();
            left = std::make_shared<BinaryExpr>(left, op, right);
        } else {
            break;
        }
    }

    return left;
}

ExprPtr Parser::parseMultiplicativeExpression() {
    ExprPtr left = parseUnaryExpression();

    while (check(TokenType::OPERATOR)) {
        std::string op = current_.value;
        if (op == "*" || op == "/" || op == "%") {
            advance();
            ExprPtr right = parseUnaryExpression();
            left = std::make_shared<BinaryExpr>(left, op, right);
        } else {
            break;
        }
    }

    return left;
}

ExprPtr Parser::parseUnaryExpression() {
    if (check(TokenType::OPERATOR)) {
        std::string op = current_.value;
        if (op == "-" || op == "+") {
            advance();
            ExprPtr operand = parseUnaryExpression();
            return std::make_shared<UnaryExpr>(op, operand);
        }
    }

    return parsePrimaryExpression();
}

ExprPtr Parser::parsePrimaryExpression() {
    // 字面量
    if (check(TokenType::INT_LITERAL)) {
        Value value(std::stoi(current_.value));
        advance();
        return std::make_shared<LiteralExpr>(value);
    }

    if (check(TokenType::FLOAT_LITERAL)) {
        Value value(std::stod(current_.value));
        advance();
        return std::make_shared<LiteralExpr>(value);
    }

    if (check(TokenType::STRING_LITERAL)) {
        Value value(current_.value);
        advance();
        return std::make_shared<LiteralExpr>(value);
    }

    // 函数调用
    if (check(TokenType::IDENTIFIER)) {
        std::string name = current_.value;
        advance();

        // 检查是否是函数调用（后面跟着左括号）
        if (check(TokenType::LEFT_PAREN)) {
            advance();

            std::vector<ExprPtr> args;
            // 解析参数列表
            if (!check(TokenType::RIGHT_PAREN)) {
                args.push_back(parseExpression());
                while (match(TokenType::COMMA)) {
                    args.push_back(parseExpression());
                }
            }
            expect(TokenType::RIGHT_PAREN, "Expected ')' after function arguments");

            return std::make_shared<FunctionCallExpr>(name, args);
        }

        // 不是函数调用，可能是列引用
        // 检查是否是 table.column 形式
        if (check(TokenType::DOT)) {
            advance();
            if (!check(TokenType::IDENTIFIER) && !check(TokenType::KEYWORD)) {
                throw MiniSQLException(ErrorCode::PARSER_SYNTAX_ERROR,
                    "Expected identifier after '.'");
            }
            std::string table = name;
            std::string column = current_.value;
            advance();
            return std::make_shared<ColumnRef>(table, column);
        }

        // 普通列引用
        return std::make_shared<ColumnRef>("", name);
    }

    // 括号表达式
    if (check(TokenType::LEFT_PAREN)) {
        advance();
        ExprPtr expr = parseExpression();
        expect(TokenType::RIGHT_PAREN, "Expected ')'");
        return expr;
    }

    throw MiniSQLException(ErrorCode::PARSER_SYNTAX_ERROR,
        "Unexpected token in expression: " + current_.value);
}

std::shared_ptr<TableRef> Parser::parseTableRef() {
    throw MiniSQLException(ErrorCode::PARSER_SYNTAX_ERROR, "TableRef not implemented yet");
}

std::shared_ptr<JoinClause> Parser::parseJoinClause() {
    throw MiniSQLException(ErrorCode::PARSER_SYNTAX_ERROR, "JoinClause not implemented yet");
}

std::vector<ExprPtr> Parser::parseExpressionList() {
    std::vector<ExprPtr> exprs;
    exprs.push_back(parseExpression());
    while (match(TokenType::COMMA)) {
        exprs.push_back(parseExpression());
    }
    return exprs;
}

std::vector<std::string> Parser::parseIdentifierList() {
    std::vector<std::string> ids;
    if (check(TokenType::IDENTIFIER)) {
        ids.push_back(current_.value);
        advance();
        while (match(TokenType::COMMA)) {
            if (check(TokenType::IDENTIFIER)) {
                ids.push_back(current_.value);
                advance();
            }
        }
    }
    return ids;
}

}  // namespace parser
}  // namespace minisql
