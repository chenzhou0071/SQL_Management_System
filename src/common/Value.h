#pragma once

#include "Type.h"
#include <variant>
#include <string>
#include <map>
#include <vector>
#include <cstdint>

namespace minisql {

// ============================================================
// 值对象 - 使用 variant 支持多种类型
// ============================================================
class Value {
public:
    using int_t = int64_t;
    using double_t = double;
    using string_t = std::string;

    Value();
    Value(DataType type);
    Value(int32_t val);
    Value(int64_t val);
    Value(double val);
    Value(const std::string& val);
    Value(const char* val);
    Value(bool val);

    // 获取值
    int64_t getInt() const;
    double getDouble() const;
    std::string getString() const;
    bool getBool() const;

    // 类型相关
    DataType getType() const { return type_; }
    bool isNull() const { return isNull_; }
    bool isNumeric() const { return TypeUtils::isNumeric(type_); }
    bool isString() const { return TypeUtils::isString(type_); }

    // 设置值
    void setNull();
    void setInt(int64_t val);
    void setDouble(double val);
    void setString(const std::string& val);
    void setBool(bool val);

    // 序列化/反序列化
    std::string toString() const;
    std::string toSQLString() const;  // 生成可执行的 SQL 字符串
    static Value parseFrom(const std::string& str, DataType type);

    // 比较运算
    int compareTo(const Value& other) const;
    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const;
    bool operator<(const Value& other) const;
    bool operator<=(const Value& other) const;
    bool operator>(const Value& other) const;
    bool operator>=(const Value& other) const;

private:
    DataType type_ = DataType::NULL_TYPE;
    bool isNull_ = true;
    std::variant<int64_t, double, std::string> value_;
};

// 行数据（多条列值）
using Row = std::vector<Value>;

// 列名校射
using RowData = std::map<std::string, Value>;

}  // namespace minisql
