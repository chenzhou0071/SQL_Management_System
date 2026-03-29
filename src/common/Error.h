#pragma once

#include <string>
#include <exception>
#include <vector>

namespace minisql {

// ============================================================
// 错误码定义
// ============================================================
enum class ErrorCode : int {
    SUCCESS = 0,

    // 通用错误 (1-99)
    UNKNOWN_ERROR = 1,

    // 解析层错误 (1000-1999)
    LEXER_ERROR_BASE = 1000,
    LEXER_UNTERMINATED_STRING = 1001,
    LEXER_INVALID_CHARACTER = 1002,
    LEXER_UNEXPECTED_EOF = 1003,

    PARSER_ERROR_BASE = 1100,
    PARSER_SYNTAX_ERROR = 1101,
    PARSER_SEMANTIC_ERROR = 1102,
    PARSER_UNEXPECTED_TOKEN = 1103,
    PARSER_MISSING_TOKEN = 1104,
    PARSER_INVALID_EXPRESSION = 1105,

    // 存储层错误 (2000-2999)
    STORAGE_ERROR_BASE = 2000,
    STORAGE_FILE_NOT_FOUND = 2001,
    STORAGE_FILE_CORRUPTED = 2002,
    STORAGE_FILE_IO_ERROR = 2003,
    STORAGE_DUPLICATE_KEY = 2004,
    STORAGE_CONSTRAINT_VIOLATION = 2005,
    STORAGE_TABLE_NOT_FOUND = 2006,
    STORAGE_DATABASE_NOT_FOUND = 2007,
    STORAGE_DATABASE_EXISTS = 2008,
    STORAGE_TABLE_EXISTS = 2009,

    // 执行层错误 (3000-3999)
    EXEC_ERROR_BASE = 3000,
    EXEC_TABLE_NOT_FOUND = 3001,
    EXEC_COLUMN_NOT_FOUND = 3002,
    EXEC_TYPE_MISMATCH = 3003,
    EXEC_DIVISION_BY_ZERO = 3004,
    EXEC_NULL_VALUE = 3005,
    EXEC_INVALID_VALUE = 3006,
    EXEC_DUPLICATE_ENTRY = 3007,
    EXEC_ROW_NOT_FOUND = 3008,
    EXEC_CONSTRAINT_VIOLATION = 3009,
    EXEC_FOREIGN_KEY_VIOLATION = 3010,

    // 索引错误 (4000-4999)
    INDEX_ERROR_BASE = 4000,
    INDEX_NOT_FOUND = 4001,
    INDEX_EXISTS = 4002,
    INDEX_INVALID_KEY = 4003,

    // 事务错误 (5000-5999)
    TRANSACTION_ERROR_BASE = 5000,
    TRANSACTION_NOT_ACTIVE = 5001,
    TRANSACTION_ALREADY_ACTIVE = 5002,
    TRANSACTION_ROLLBACK = 5003,

    // 视图/存储过程/触发器错误 (6000-6999)
    VIEW_ERROR_BASE = 6000,
    VIEW_NOT_FOUND = 6001,
    VIEW_EXISTS = 6002,

    PROCEDURE_ERROR_BASE = 6100,
    PROCEDURE_NOT_FOUND = 6101,
    PROCEDURE_EXISTS = 6102,

    TRIGGER_ERROR_BASE = 6200,
    TRIGGER_NOT_FOUND = 6201,
    TRIGGER_EXISTS = 6202
};

// ============================================================
// MiniSQL 异常类
// ============================================================
class MiniSQLException : public std::exception {
public:
    MiniSQLException() : code_(ErrorCode::SUCCESS), message_(""), sqlState_("00000") {}
    explicit MiniSQLException(ErrorCode code, const std::string& message);

    ErrorCode getCode() const { return code_; }
    const std::string& getMessage() const { return message_; }
    const std::string& getSQLState() const { return sqlState_; }
    const std::string& getDetail() const { return detail_; }

    void setDetail(const std::string& detail) { detail_ = detail; }

    const char* what() const noexcept override;

    std::string toString() const;

private:
    ErrorCode code_;
    std::string message_;
    std::string sqlState_;
    std::string detail_;

    static std::string errorCodeToSQLState(ErrorCode code);
};

// ============================================================
// 解析错误位置
// ============================================================
struct ParseError {
    int line;
    int column;
    std::string message;
    std::string token;  // 出问题的 token

    std::string toString() const;
};

// ============================================================
// Result 包装类（用于返回值）
// ============================================================
template<typename T>
class Result {
public:
    Result() : success_(true), value_(nullptr) {}
    Result(T* value) : success_(true), value_(value) {}
    Result(const T& value) : success_(true), value_(new T(value)) {}
    Result(const MiniSQLException& error) : success_(false), error_(error) {}

    // 禁止拷贝
    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;

    // 移动构造
    Result(Result&& other) noexcept : success_(other.success_), value_(other.value_), error_(std::move(other.error_)) {
        other.value_ = nullptr;
    }

    ~Result() { delete value_; }

    bool isSuccess() const { return success_; }
    bool isError() const { return !success_; }
    T* getValue() const { return value_; }
    T* release() { T* v = value_; value_ = nullptr; return v; }
    const MiniSQLException& getError() const { return error_; }

    T* operator->() const { return value_; }
    T& operator*() const { return *value_; }
    explicit operator bool() const { return success_; }

private:
    bool success_;
    T* value_;
    MiniSQLException error_;
};

// 特化 for void
template<>
class Result<void> {
public:
    Result() : success_(true) {}
    Result(const MiniSQLException& error) : success_(false), error_(error) {}

    bool isSuccess() const { return success_; }
    bool isError() const { return !success_; }
    const MiniSQLException& getError() const { return error_; }
    explicit operator bool() const { return success_; }

private:
    bool success_;
    MiniSQLException error_{ErrorCode::SUCCESS, ""};
};

}  // namespace minisql
