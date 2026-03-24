#include "Type.h"
#include <sstream>

using namespace minisql;

// ============================================================
// ColumnDef 方法
// ============================================================
std::string ColumnDef::toString() const {
    std::stringstream ss;
    ss << name << " " << TypeUtils::typeToString(type);
    if (length > 0 && (type == DataType::CHAR || type == DataType::VARCHAR)) {
        ss << "(" << length << ")";
    }
    if (type == DataType::DECIMAL && precision > 0) {
        ss << "(" << precision << "," << scale << ")";
    }
    if (primaryKey) ss << " PRIMARY KEY";
    if (notNull) ss << " NOT NULL";
    if (unique && !primaryKey) ss << " UNIQUE";
    if (autoIncrement) ss << " AUTO_INCREMENT";
    if (hasDefault) ss << " DEFAULT " << defaultValue;
    return ss.str();
}

// ============================================================
// IndexDef 方法
// ============================================================
std::string IndexDef::toString() const {
    std::stringstream ss;
    ss << "INDEX " << name << " (";
    for (size_t i = 0; i < columns.size(); i++) {
        if (i > 0) ss << ", ";
        ss << columns[i];
    }
    ss << ")";
    if (unique) ss << " UNIQUE";
    return ss.str();
}

// ============================================================
// ForeignKeyDef 方法
// ============================================================
std::string ForeignKeyDef::toString() const {
    std::stringstream ss;
    ss << "FOREIGN KEY " << name << " (" << column << ") REFERENCES " << refTable << "(" << refColumn << ")";
    return ss.str();
}

// ============================================================
// TableDef 方法
// ============================================================
std::string TableDef::toString() const {
    std::stringstream ss;
    ss << "Table: " << name << "\n";
    for (const auto& col : columns) {
        ss << "  " << col.toString() << "\n";
    }
    return ss.str();
}

// ============================================================
// TypeUtils 方法
// ============================================================
std::string TypeUtils::typeToString(DataType type) {
    switch (type) {
        case DataType::TINYINT:   return "TINYINT";
        case DataType::SMALLINT:  return "SMALLINT";
        case DataType::INT:        return "INT";
        case DataType::BIGINT:     return "BIGINT";
        case DataType::FLOAT:      return "FLOAT";
        case DataType::DOUBLE:     return "DOUBLE";
        case DataType::DECIMAL:    return "DECIMAL";
        case DataType::CHAR:       return "CHAR";
        case DataType::VARCHAR:    return "VARCHAR";
        case DataType::TEXT:       return "TEXT";
        case DataType::DATE:       return "DATE";
        case DataType::TIME:       return "TIME";
        case DataType::DATETIME:   return "DATETIME";
        case DataType::TIMESTAMP:  return "TIMESTAMP";
        case DataType::BOOLEAN:    return "BOOLEAN";
        case DataType::NULL_TYPE:  return "NULL";
        default:                   return "UNKNOWN";
    }
}

DataType TypeUtils::stringToType(const std::string& str) {
    std::string s = str;
    // 转小写
    for (auto& c : s) c = static_cast<char>(std::tolower(c));

    if (s == "tinyint") return DataType::TINYINT;
    if (s == "smallint") return DataType::SMALLINT;
    if (s == "int" || s == "integer") return DataType::INT;
    if (s == "bigint") return DataType::BIGINT;
    if (s == "float" || s == "real") return DataType::FLOAT;
    if (s == "double" || s == "double precision") return DataType::DOUBLE;
    if (s == "decimal" || s == "dec" || s == "numeric") return DataType::DECIMAL;
    if (s == "char") return DataType::CHAR;
    if (s == "varchar" || s == "character varying") return DataType::VARCHAR;
    if (s == "text" || s == "tinytext" || s == "mediumtext" || s == "longtext") return DataType::TEXT;
    if (s == "date") return DataType::DATE;
    if (s == "time") return DataType::TIME;
    if (s == "datetime") return DataType::DATETIME;
    if (s == "timestamp") return DataType::TIMESTAMP;
    if (s == "boolean" || s == "bool") return DataType::BOOLEAN;
    if (s == "null") return DataType::NULL_TYPE;

    return DataType::UNKNOWN;
}

int TypeUtils::getTypeSize(DataType type, int length) {
    switch (type) {
        case DataType::TINYINT:   return 1;
        case DataType::SMALLINT:  return 2;
        case DataType::INT:        return 4;
        case DataType::BIGINT:     return 8;
        case DataType::FLOAT:      return 4;
        case DataType::DOUBLE:     return 8;
        case DataType::DECIMAL:    return 16;  // 近似值
        case DataType::CHAR:       return length > 0 ? length : 1;
        case DataType::VARCHAR:    return length > 0 ? length : 255;
        case DataType::TEXT:       return 65535;
        case DataType::DATE:       return 3;   // YYYY-MM-DD
        case DataType::TIME:       return 3;   // HH:MM:SS
        case DataType::DATETIME:   return 8;   // YYYY-MM-DD HH:MM:SS
        case DataType::TIMESTAMP:  return 4;   // Unix timestamp
        case DataType::BOOLEAN:    return 1;
        default:                   return 0;
    }
}

bool TypeUtils::isNumeric(DataType type) {
    return type == DataType::TINYINT || type == DataType::SMALLINT ||
           type == DataType::INT || type == DataType::BIGINT ||
           type == DataType::FLOAT || type == DataType::DOUBLE ||
           type == DataType::DECIMAL || type == DataType::BOOLEAN;
}

bool TypeUtils::isString(DataType type) {
    return type == DataType::CHAR || type == DataType::VARCHAR ||
           type == DataType::TEXT;
}

bool TypeUtils::isDateTime(DataType type) {
    return type == DataType::DATE || type == DataType::TIME ||
           type == DataType::DATETIME || type == DataType::TIMESTAMP;
}

bool TypeUtils::isComparable(DataType a, DataType b) {
    if (a == b) return true;
    // NULL 可以和任何类型比较
    if (a == DataType::NULL_TYPE || b == DataType::NULL_TYPE) return true;
    // 数值类型之间可以比较
    if (isNumeric(a) && isNumeric(b)) return true;
    // 字符串类型之间可以比较
    if (isString(a) && isString(b)) return true;
    // 日期时间类型之间可以比较
    if (isDateTime(a) && isDateTime(b)) return true;
    return false;
}

bool TypeUtils::isImplicitlyConvertible(DataType from, DataType to) {
    if (from == to) return true;
    if (from == DataType::NULL_TYPE) return true;
    if (to == DataType::NULL_TYPE) return true;

    // 整数类型之间可以转换
    static const DataType intTypes[] = {
        DataType::TINYINT, DataType::SMALLINT, DataType::INT, DataType::BIGINT
    };
    for (size_t i = 0; i < 4; i++) {
        if (from == intTypes[i]) {
            for (size_t j = 0; j < 4; j++) {
                if (to == intTypes[j]) return true;
            }
        }
    }

    // 浮点类型之间可以转换
    static const DataType floatTypes[] = {
        DataType::FLOAT, DataType::DOUBLE, DataType::DECIMAL
    };
    for (size_t i = 0; i < 3; i++) {
        if (from == floatTypes[i]) {
            for (size_t j = 0; j < 3; j++) {
                if (to == floatTypes[j]) return true;
            }
        }
    }

    // 字符串可以隐式转为字符串类型
    if (isString(from) && isString(to)) return true;

    return false;
}
