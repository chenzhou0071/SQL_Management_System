#pragma once

#include <string>
#include <map>
#include <vector>

namespace minisql {

// ============================================================
// 数据类型枚举
// ============================================================
enum class DataType {
    // 整数类型
    TINYINT,
    SMALLINT,
    INT,
    BIGINT,

    // 浮点类型
    FLOAT,
    DOUBLE,
    DECIMAL,

    // 字符串类型
    CHAR,
    VARCHAR,
    TEXT,

    // 日期时间类型
    DATE,
    TIME,
    DATETIME,
    TIMESTAMP,

    // 其他
    BOOLEAN,
    NULL_TYPE,
    UNKNOWN
};

// ============================================================
// 约束类型
// ============================================================
enum class ConstraintType {
    NONE,
    PRIMARY_KEY,
    UNIQUE,
    NOT_NULL,
    DEFAULT,
    AUTO_INCREMENT,
    FOREIGN_KEY,
    CHECK
};

// ============================================================
// 索引类型
// ============================================================
enum class IndexType {
    BTREE,
    HASH
};

// ============================================================
// 索引定义
// ============================================================
struct IndexDef {
    std::string name;           // 索引名称
    std::string tableName;      // 所属表
    std::vector<std::string> columns;  // 索引列（支持复合索引）
    IndexType type = IndexType::BTREE;
    bool unique = false;        // 是否唯一索引

    std::string toString() const;
};

// ============================================================
// 外键定义
// ============================================================
struct ForeignKeyDef {
    std::string name;           // 外键名称
    std::string column;         // 当前表列
    std::string refTable;       // 引用表
    std::string refColumn;      // 引用列
    std::string onDelete;       // DELETE 时动作：CASCADE, SET NULL, RESTRICT, NO ACTION
    std::string onUpdate;       // UPDATE 时动作

    std::string toString() const;
};

// ============================================================
// 列定义
// ============================================================
struct ColumnDef {
    std::string name;
    DataType type;
    int length = 0;       // VARCHAR/CHAR 长度
    int precision = 0;    // DECIMAL 总精度
    int scale = 0;        // DECIMAL 小数位

    bool notNull = false;
    bool primaryKey = false;
    bool unique = false;
    bool autoIncrement = false;
    bool hasDefault = false;

    std::string defaultValue;  // 默认值字符串
    std::string comment;        // 注释

    std::string toString() const;
};

// ============================================================
// 表定义
// ============================================================
struct TableDef {
    std::string name;
    std::string database;
    std::string engine = "MiniSQL";
    std::string comment;
    std::vector<ColumnDef> columns;
    std::vector<IndexDef> indexes;      // 表级索引定义
    std::vector<ForeignKeyDef> foreignKeys;  // 外键定义

    std::string toString() const;
};

// ============================================================
// 类型工具函数
// ============================================================
class TypeUtils {
public:
    // 获取类型名称
    static std::string typeToString(DataType type);

    // 解析字符串为类型
    static DataType stringToType(const std::string& str);

    // 获取类型占用字节数
    static int getTypeSize(DataType type, int length = 0);

    // 判断是否为数值类型
    static bool isNumeric(DataType type);

    // 判断是否为字符串类型
    static bool isString(DataType type);

    // 判断是否为日期时间类型
    static bool isDateTime(DataType type);

    // 判断两个类型是否可以比较
    static bool isComparable(DataType a, DataType b);

    // 判断两个类型是否可以隐式转换
    static bool isImplicitlyConvertible(DataType from, DataType to);
};

}  // namespace minisql
