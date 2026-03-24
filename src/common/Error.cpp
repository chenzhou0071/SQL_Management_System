#include "Error.h"
#include <sstream>

using namespace minisql;

// ============================================================
// MiniSQLException 实现
// ============================================================
MiniSQLException::MiniSQLException(ErrorCode code, const std::string& message)
    : code_(code), message_(message), sqlState_(errorCodeToSQLState(code)) {}

const char* MiniSQLException::what() const noexcept {
    return message_.c_str();
}

std::string MiniSQLException::toString() const {
    std::stringstream ss;
    ss << "[" << static_cast<int>(code_) << "] ";
    if (!sqlState_.empty()) {
        ss << sqlState_ << " ";
    }
    ss << message_;
    if (!detail_.empty()) {
        ss << " (" << detail_ << ")";
    }
    return ss.str();
}

std::string MiniSQLException::errorCodeToSQLState(ErrorCode code) {
    switch (code) {
        // 解析层
        case ErrorCode::LEXER_UNTERMINATED_STRING:
        case ErrorCode::LEXER_INVALID_CHARACTER:
        case ErrorCode::LEXER_UNEXPECTED_EOF:
        case ErrorCode::PARSER_SYNTAX_ERROR:
        case ErrorCode::PARSER_UNEXPECTED_TOKEN:
        case ErrorCode::PARSER_MISSING_TOKEN:
        case ErrorCode::PARSER_INVALID_EXPRESSION:
            return "42000";  // Syntax error or access rule violation

        case ErrorCode::PARSER_SEMANTIC_ERROR:
            return "42001";  // Syntax error

        // 存储层
        case ErrorCode::STORAGE_FILE_NOT_FOUND:
        case ErrorCode::STORAGE_TABLE_NOT_FOUND:
        case ErrorCode::STORAGE_DATABASE_NOT_FOUND:
        case ErrorCode::INDEX_NOT_FOUND:
        case ErrorCode::VIEW_NOT_FOUND:
        case ErrorCode::PROCEDURE_NOT_FOUND:
        case ErrorCode::TRIGGER_NOT_FOUND:
            return "42S02";  // Base table or view not found

        case ErrorCode::STORAGE_FILE_CORRUPTED:
        case ErrorCode::STORAGE_FILE_IO_ERROR:
            return "22018";  // Data type mismatch

        case ErrorCode::STORAGE_DUPLICATE_KEY:
        case ErrorCode::STORAGE_CONSTRAINT_VIOLATION:
        case ErrorCode::INDEX_EXISTS:
        case ErrorCode::STORAGE_DATABASE_EXISTS:
        case ErrorCode::STORAGE_TABLE_EXISTS:
        case ErrorCode::VIEW_EXISTS:
        case ErrorCode::PROCEDURE_EXISTS:
        case ErrorCode::TRIGGER_EXISTS:
            return "23000";  // Integrity constraint violation

        // 执行层
        case ErrorCode::EXEC_TABLE_NOT_FOUND:
        case ErrorCode::EXEC_COLUMN_NOT_FOUND:
            return "42S02";

        case ErrorCode::EXEC_TYPE_MISMATCH:
            return "22005";  // Error in assignment

        case ErrorCode::EXEC_DIVISION_BY_ZERO:
            return "22012";  // Division by zero

        case ErrorCode::EXEC_NULL_VALUE:
            return "23000";

        case ErrorCode::EXEC_INVALID_VALUE:
            return "22003";  // Numeric value out of range

        case ErrorCode::EXEC_DUPLICATE_ENTRY:
        case ErrorCode::EXEC_CONSTRAINT_VIOLATION:
        case ErrorCode::EXEC_FOREIGN_KEY_VIOLATION:
            return "23000";

        case ErrorCode::EXEC_ROW_NOT_FOUND:
            return "02000";  // No data

        // 索引错误
        case ErrorCode::INDEX_INVALID_KEY:
            return "23000";

        // 事务错误
        case ErrorCode::TRANSACTION_NOT_ACTIVE:
            return "25000";  // Invalid transaction state

        case ErrorCode::TRANSACTION_ALREADY_ACTIVE:
            return "25001";  // Invalid transaction state

        case ErrorCode::TRANSACTION_ROLLBACK:
            return "40000";  // Rollback

        default:
            return "HY000";  // General error
    }
}

// ============================================================
// ParseError 实现
// ============================================================
std::string ParseError::toString() const {
    std::stringstream ss;
    ss << "Line " << line << ", Column " << column << ": " << message;
    if (!token.empty()) {
        ss << " (near '" << token << "')";
    }
    return ss.str();
}
