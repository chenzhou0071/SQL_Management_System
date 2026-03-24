#include "../src/common/Type.h"
#include "../src/common/Value.h"
#include "../src/common/Error.h"
#include "../src/common/Logger.h"
#include "test_utils.h"
#include <iostream>

using namespace minisql;

// ============================================================
// Logger 测试
// ============================================================
TEST(test_logger_init) {
    Logger::getInstance().init("logs", LogLevel::DEBUG);
    LOG_DEBUG("Test", "Debug message");
    LOG_INFO("Test", "Info message");
    LOG_WARN("Test", "Warning message");
    LOG_ERROR("Test", "Error message");
    ASSERT_TRUE(true);
}

// ============================================================
// Type 测试
// ============================================================
TEST(test_type_string_conversion) {
    ASSERT_STREQ(TypeUtils::typeToString(DataType::INT).c_str(), "INT");
    ASSERT_STREQ(TypeUtils::typeToString(DataType::VARCHAR).c_str(), "VARCHAR");
    ASSERT_STREQ(TypeUtils::typeToString(DataType::DATE).c_str(), "DATE");

    ASSERT_TRUE(TypeUtils::stringToType("int") == DataType::INT);
    ASSERT_TRUE(TypeUtils::stringToType("varchar") == DataType::VARCHAR);
    ASSERT_TRUE(TypeUtils::stringToType("date") == DataType::DATE);
    ASSERT_TRUE(TypeUtils::stringToType("INT") == DataType::INT);
    ASSERT_TRUE(TypeUtils::stringToType("INTEGER") == DataType::INT);
}

TEST(test_type_size) {
    ASSERT_EQ(TypeUtils::getTypeSize(DataType::TINYINT), 1);
    ASSERT_EQ(TypeUtils::getTypeSize(DataType::SMALLINT), 2);
    ASSERT_EQ(TypeUtils::getTypeSize(DataType::INT), 4);
    ASSERT_EQ(TypeUtils::getTypeSize(DataType::BIGINT), 8);
    ASSERT_EQ(TypeUtils::getTypeSize(DataType::FLOAT), 4);
    ASSERT_EQ(TypeUtils::getTypeSize(DataType::DOUBLE), 8);
    ASSERT_EQ(TypeUtils::getTypeSize(DataType::CHAR, 10), 10);
    ASSERT_EQ(TypeUtils::getTypeSize(DataType::VARCHAR, 255), 255);
}

TEST(test_type_category) {
    ASSERT_TRUE(TypeUtils::isNumeric(DataType::INT));
    ASSERT_TRUE(TypeUtils::isNumeric(DataType::BIGINT));
    ASSERT_TRUE(TypeUtils::isNumeric(DataType::FLOAT));
    ASSERT_TRUE(TypeUtils::isNumeric(DataType::DOUBLE));
    ASSERT_FALSE(TypeUtils::isNumeric(DataType::VARCHAR));

    ASSERT_TRUE(TypeUtils::isString(DataType::CHAR));
    ASSERT_TRUE(TypeUtils::isString(DataType::VARCHAR));
    ASSERT_TRUE(TypeUtils::isString(DataType::TEXT));
    ASSERT_FALSE(TypeUtils::isString(DataType::INT));

    ASSERT_TRUE(TypeUtils::isDateTime(DataType::DATE));
    ASSERT_TRUE(TypeUtils::isDateTime(DataType::DATETIME));
    ASSERT_FALSE(TypeUtils::isDateTime(DataType::INT));
}

TEST(test_column_def) {
    ColumnDef col;
    col.name = "id";
    col.type = DataType::INT;
    col.primaryKey = true;
    col.notNull = true;

    std::string s = col.toString();
    ASSERT_TRUE(s.find("id") != std::string::npos);
    ASSERT_TRUE(s.find("INT") != std::string::npos);
    ASSERT_TRUE(s.find("PRIMARY KEY") != std::string::npos);
}

// ============================================================
// Value 测试
// ============================================================
TEST(test_value_int) {
    Value v(42);
    ASSERT_FALSE(v.isNull());
    ASSERT_EQ(v.getInt(), 42);
    ASSERT_STREQ(v.toString().c_str(), "42");
}

TEST(test_value_double) {
    Value v(3.14159);
    ASSERT_FALSE(v.isNull());
    ASSERT_EQ(v.getDouble(), 3.14159);
}

TEST(test_value_string) {
    Value v("hello");
    ASSERT_FALSE(v.isNull());
    ASSERT_STREQ(v.getString().c_str(), "hello");
}

TEST(test_value_null) {
    Value v;
    ASSERT_TRUE(v.isNull());
    ASSERT_STREQ(v.toString().c_str(), "NULL");
}

TEST(test_value_comparison) {
    Value a(10);
    Value b(20);
    Value c(10);

    ASSERT_TRUE(a == c);
    ASSERT_TRUE(a != b);
    ASSERT_TRUE(a < b);
    ASSERT_TRUE(b > a);
    ASSERT_TRUE(a <= c);
    ASSERT_TRUE(a >= c);
}

TEST(test_value_parse_from) {
    Value v1 = Value::parseFrom("123", DataType::INT);
    ASSERT_FALSE(v1.isNull());
    ASSERT_EQ(v1.getInt(), 123);

    Value v2 = Value::parseFrom("3.14", DataType::DOUBLE);
    ASSERT_FALSE(v2.isNull());

    Value v3 = Value::parseFrom("NULL", DataType::INT);
    ASSERT_TRUE(v3.isNull());

    // 测试 \N 格式的 NULL
    Value v4 = Value::parseFrom("\\N", DataType::VARCHAR);
    ASSERT_TRUE(v4.isNull());

    Value v5 = Value::parseFrom("null", DataType::INT);
    ASSERT_TRUE(v5.isNull());
}

TEST(test_value_sql_string) {
    Value v("Tom's car");
    ASSERT_STREQ(v.toSQLString().c_str(), "'Tom''s car'");

    Value n(42);
    ASSERT_STREQ(n.toSQLString().c_str(), "42");

    // 测试 NULL 输出为 \N
    Value nullVal;
    ASSERT_STREQ(nullVal.toSQLString().c_str(), "\\N");
}

// ============================================================
// Error 测试
// ============================================================
TEST(test_error_exception) {
    MiniSQLException err(ErrorCode::EXEC_TABLE_NOT_FOUND, "Table 'users' not found");
    err.setDetail("database: test_db");
    ASSERT_EQ(static_cast<int>(err.getCode()), 3001);
    ASSERT_TRUE(err.toString().find("users") != std::string::npos);
    ASSERT_TRUE(err.toString().find("test_db") != std::string::npos);
    ASSERT_STREQ(err.getSQLState().c_str(), "42S02");
}

// ============================================================
// 入口
// ============================================================
int main() {
    std::cout << "Running MiniSQL Common Module Tests..." << std::endl;

    test_logger_init();
    test_type_string_conversion();
    test_type_size();
    test_type_category();
    test_column_def();
    test_value_int();
    test_value_double();
    test_value_string();
    test_value_null();
    test_value_comparison();
    test_value_parse_from();
    test_value_sql_string();
    test_error_exception();

    RUN_TESTS;

    std::cout << "\n按回车键退出..." << std::endl;
    std::cin.get();
    return 0;
}
