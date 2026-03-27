#include "Value.h"
#include <sstream>
#include <algorithm>
#include <cstdint>

using namespace minisql;

Value::Value() : type_(DataType::NULL_TYPE), isNull_(true) {}

Value::Value(DataType type) : type_(type), isNull_(false) {
    if (TypeUtils::isNumeric(type_)) {
        value_.emplace<int64_t>(0);
    } else {
        value_.emplace<std::string>("");
    }
}

Value::Value(int32_t val) : type_(DataType::INT), isNull_(false) {
    value_.emplace<int64_t>(val);
}

Value::Value(int64_t val) : type_(DataType::BIGINT), isNull_(false) {
    value_.emplace<int64_t>(val);
}

Value::Value(double val) : type_(DataType::DOUBLE), isNull_(false) {
    value_.emplace<double>(val);
}

Value::Value(const std::string& val) : type_(DataType::VARCHAR), isNull_(false) {
    value_.emplace<std::string>(val);
}

Value::Value(const char* val) : type_(DataType::VARCHAR), isNull_(false) {
    value_.emplace<std::string>(std::string(val));
}

Value::Value(bool val) : type_(DataType::BOOLEAN), isNull_(false) {
    value_.emplace<std::string>(val ? "true" : "false");
}

void Value::setNull() {
    isNull_ = true;
    type_ = DataType::NULL_TYPE;
}

void Value::setInt(int64_t val) {
    isNull_ = false;
    type_ = DataType::BIGINT;
    value_.emplace<int64_t>(val);
}

void Value::setDouble(double val) {
    isNull_ = false;
    type_ = DataType::DOUBLE;
    value_.emplace<double>(val);
}

void Value::setString(const std::string& val) {
    isNull_ = false;
    type_ = DataType::VARCHAR;
    value_.emplace<std::string>(val);
}

void Value::setBool(bool val) {
    isNull_ = false;
    type_ = DataType::BOOLEAN;
    value_.emplace<std::string>(val ? "true" : "false");
}

int64_t Value::getInt() const {
    if (isNull_) return 0;
    if (std::holds_alternative<int64_t>(value_)) {
        return std::get<int64_t>(value_);
    } else if (std::holds_alternative<double>(value_)) {
        return static_cast<int64_t>(std::get<double>(value_));
    } else {
        return std::stoll(std::get<std::string>(value_));
    }
}

double Value::getDouble() const {
    if (isNull_) return 0.0;
    if (std::holds_alternative<double>(value_)) {
        return std::get<double>(value_);
    } else if (std::holds_alternative<int64_t>(value_)) {
        return static_cast<double>(std::get<int64_t>(value_));
    } else {
        return std::stod(std::get<std::string>(value_));
    }
}

std::string Value::getString() const {
    if (isNull_) return "NULL";
    if (std::holds_alternative<std::string>(value_)) {
        return std::get<std::string>(value_);
    } else if (std::holds_alternative<int64_t>(value_)) {
        return std::to_string(std::get<int64_t>(value_));
    } else {
        return std::to_string(std::get<double>(value_));
    }
}

bool Value::getBool() const {
    if (isNull_) return false;
    if (std::holds_alternative<std::string>(value_)) {
        std::string val = std::get<std::string>(value_);
        std::transform(val.begin(), val.end(), val.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return val == "true" || val == "1" || val == "yes";
    } else if (std::holds_alternative<int64_t>(value_)) {
        return std::get<int64_t>(value_) != 0;
    } else {
        return std::get<double>(value_) != 0.0;
    }
}

std::string Value::toString() const {
    if (isNull_) return "NULL";
    return getString();
}

std::string Value::toSQLString() const {
    if (isNull_) return "\\N";
    if (type_ == DataType::INT || type_ == DataType::BIGINT ||
        type_ == DataType::SMALLINT || type_ == DataType::TINYINT ||
        type_ == DataType::FLOAT || type_ == DataType::DOUBLE ||
        type_ == DataType::DECIMAL) {
        return getString();
    }
    // 字符串类型需要加引号并转义
    std::string s = getString();
    std::string result;
    result.reserve(s.size() + 4);
    result.push_back('\'');
    for (char c : s) {
        if (c == '\'') result.push_back('\'');  // 转义单引号
        result.push_back(c);
    }
    result.push_back('\'');
    return result;
}

Value Value::parseFrom(const std::string& str, DataType type) {
    Value val;
    val.type_ = type;

    if (str == "NULL" || str == "null" || str == "\\N" || str == "\\\\N" || str.empty()) {
        val.isNull_ = true;
        return val;
    }

    val.isNull_ = false;

    switch (type) {
        case DataType::TINYINT:
        case DataType::SMALLINT:
        case DataType::INT:
        case DataType::BIGINT:
            val.value_.emplace<int64_t>(std::stoll(str));
            break;
        case DataType::FLOAT:
        case DataType::DOUBLE:
        case DataType::DECIMAL:
            val.value_.emplace<double>(std::stod(str));
            break;
        case DataType::BOOLEAN:
            {
                std::string lower = str;
                std::transform(lower.begin(), lower.end(), lower.begin(),
                    [](unsigned char c) { return std::tolower(c); });
                val.value_.emplace<std::string>((lower == "true" || lower == "1" || lower == "yes") ? "true" : "false");
            }
            break;
        default:
            val.value_.emplace<std::string>(str);
            break;
    }
    return val;
}

int Value::compareTo(const Value& other) const {
    if (isNull_ && other.isNull_) return 0;
    if (isNull_) return -1;
    if (other.isNull_) return 1;

    if (TypeUtils::isNumeric(type_) && TypeUtils::isNumeric(other.type_)) {
        double a = getDouble();
        double b = other.getDouble();
        if (a < b) return -1;
        if (a > b) return 1;
        return 0;
    }

    // 字符串比较
    std::string a = getString();
    std::string b = other.getString();
    return a.compare(b);
}

bool Value::operator==(const Value& other) const { return compareTo(other) == 0; }
bool Value::operator!=(const Value& other) const { return compareTo(other) != 0; }
bool Value::operator<(const Value& other) const { return compareTo(other) < 0; }
bool Value::operator<=(const Value& other) const { return compareTo(other) <= 0; }
bool Value::operator>(const Value& other) const { return compareTo(other) > 0; }
bool Value::operator>=(const Value& other) const { return compareTo(other) >= 0; }
