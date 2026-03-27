# 执行器 (Executor) 实施计划

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**目标:** 实现完整的 SQL 查询执行引擎，支持表达式计算、数据扫描、连接、聚合和排序操作

**架构:** 采用火山模型（Volcano Model）迭代器模式，每个执行算子实现 next() 接口，支持流水线式执行

**技术栈:** C++17, nlohmann/json, 已有的存储引擎和解析器

---

## 存储结构与元数据 (Catalog) 参考

**重要:** 执行器需要与已有的存储引擎交互，以下是关键的存储接口和数据结构。

### 文件存储结构

```
data/
├── <database_name>/
│   ├── <table_name>.meta      # 表元数据 (JSON 格式)
│   ├── <table_name>.data      # 表数据 (CSV 格式)
│   ├── <table_name>.rowid     # 自增 ID 计数器
│   └── <index_name>.idx       # 索引文件 (B+ 树序列化)
```

### 核心存储接口

#### TableManager (src/storage/TableManager.h)

单例模式，提供数据库和表的 CRUD 操作：

```cpp
class TableManager {
public:
    static TableManager& getInstance();

    // 数据库操作
    Result<void> createDatabase(const std::string& dbName);
    Result<void> dropDatabase(const std::string& dbName);
    Result<std::vector<std::string>> listDatabases();
    bool databaseExists(const std::string& dbName);

    // 表操作
    Result<void> createTable(const std::string& dbName, const TableDef& tableDef);
    Result<void> dropTable(const std::string& dbName, const std::string& tableName);
    Result<bool> tableExists(const std::string& dbName, const std::string& tableName);
    Result<std::vector<std::string>> listTables(const std::string& dbName);
    Result<TableDef> getTableDef(const std::string& dbName, const std::string& tableName);

    // 数据操作
    Result<void> insertRow(const std::string& dbName, const std::string& tableName, const Row& row);
    Result<TableData> loadTable(const std::string& dbName, const std::string& tableName);
    Result<void> deleteRow(const std::string& dbName, const std::string& tableName, int rowId);
    Result<void> updateRow(const std::string& dbName, const std::string& tableName, int rowId, const Row& row);

    // 获取下一行 ID（自增）
    Result<int> getNextRowId(const std::string& dbName, const std::string& tableName);
};
```

#### 关键数据类型 (src/common/Type.h)

```cpp
// 数据类型枚举
enum class DataType {
    TINYINT, SMALLINT, INT, BIGINT,
    FLOAT, DOUBLE, DECIMAL,
    CHAR, VARCHAR, TEXT,
    DATE, TIME, DATETIME, TIMESTAMP,
    BOOLEAN, NULL_TYPE, UNKNOWN
};

// 列定义
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
    std::string defaultValue;
    std::string comment;
};

// 表定义
struct TableDef {
    std::string name;
    std::string database;
    std::string engine = "MiniSQL";
    std::string comment;
    std::vector<ColumnDef> columns;
    std::vector<IndexDef> indexes;
    std::vector<ForeignKeyDef> foreignKeys;
};

// 行数据（列值的向量）
using Row = std::vector<Value>;

// 表数据（行 ID -> 行）
using TableData = std::map<int, Row>;
```

#### Value 对象 (src/common/Value.h)

```cpp
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
    DataType getType() const;
    bool isNull() const;
    bool isNumeric() const;
    bool isString() const;

    // 设置值
    void setNull();
    void setInt(int64_t val);
    void setDouble(double val);
    void setString(const std::string& val);
    void setBool(bool val);

    // 序列化/反序列化
    std::string toString() const;
    std::string toSQLString() const;
    static Value parseFrom(const std::string& str, DataType type);

    // 比较运算
    int compareTo(const Value& other) const;
    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const;
    bool operator<(const Value& other) const;
    bool operator<=(const Value& other) const;
    bool operator>(const Value& other) const;
    bool operator>=(const Value& other) const;
};
```

#### FileIO 工具类 (src/storage/FileIO.h)

```cpp
class FileIO {
public:
    // 目录操作
    static bool exists(const std::string& path);
    static bool isDirectory(const std::string& path);
    static bool createDirectory(const std::string& path);
    static bool removeDirectory(const std::string& path);
    static std::vector<std::string> listDirectories(const std::string& path);
    static std::vector<std::string> listFiles(const std::string& path, const std::string& extension = "");

    // 文件操作
    static bool existsFile(const std::string& path);
    static bool removeFile(const std::string& path);
    static bool writeToFile(const std::string& path, const std::string& content);
    static std::string readFromFile(const std::string& path);
    static bool appendToFile(const std::string& path, const std::string& content);

    // 路径工具
    static std::string join(const std::string& base, const std::string& relative);
    static std::string getDataDir();
    static std::string getDatabaseDir(const std::string& dbName);
};
```

#### IndexManager (src/storage/IndexManager.h)

```cpp
class IndexManager {
public:
    static IndexManager& getInstance();

    // 索引操作
    Result<void> createIndex(const std::string& dbName, const std::string& tableName,
                            const std::string& indexName, const std::string& columnName,
                            IndexUsageType type);
    Result<void> dropIndex(const std::string& dbName, const std::string& indexName);

    // 查找索引
    Result<std::shared_ptr<BPlusTree>> getIndex(const std::string& dbName, const std::string& indexName);
    Result<std::shared_ptr<BPlusTree>> getPrimaryKeyIndex(const std::string& dbName, const std::string& tableName);

    // 索引路径
    std::string getIndexPath(const std::string& dbName, const std::string& indexName);

    // 保存/加载索引
    Result<void> saveIndex(const std::string& dbName, const std::string& indexName);
    Result<void> loadIndex(const std::string& dbName, const std::string& indexName);
};
```

### 文件格式示例

#### .meta 文件 (表元数据 - JSON)

```json
{
    "table_name": "users",
    "engine": "MiniSQL",
    "database": "test_db",
    "columns": [
        {"name": "id", "type": "INT", "primary_key": true},
        {"name": "name", "type": "VARCHAR", "length": 50, "not_null": true},
        {"name": "age", "type": "INT"}
    ],
    "indexes": [
        {"name": "PRIMARY", "columns": ["id"], "type": "BTREE", "unique": true}
    ],
    "foreign_keys": []
}
```

#### .data 文件 (表数据 - CSV)

```
1,'Alice',25
2,'Bob',30
3,'Charlie',\N
```

#### .rowid 文件 (自增 ID)

```
4
```

### 执行器使用存储接口的注意事项

1. **总是使用 TableManager::getInstance()** 获取单例
2. **所有操作返回 Result<T>**，需要检查 isSuccess()
3. **行 ID 从 1 开始**，不是从 0 开始
4. **NULL 值用 \N 表示** 在 CSV 文件中
5. **Value::isNull()** 检查是否为 NULL
6. **数据库名和表名区分大小写**
7. **loadTable() 返回 map<int, Row>**，key 是 rowId

---

## 任务概览

1. **表达式计算引擎** (ExpressionEvaluator) - 支持算术、逻辑、比较表达式和函数调用
2. **执行算子基类** (ExecutionOperator) - 火山模型迭代器接口
3. **表扫描算子** (TableScanOperator) - 全表扫描和索引扫描
4. **过滤算子** (FilterOperator) - WHERE/HAVING 条件执行
5. **投影算子** (ProjectOperator) - SELECT 列表计算
6. **连接算子** (NestedLoopJoinOperator) - 嵌套循环连接
7. **聚合算子** (AggregateOperator) - GROUP BY 和聚合函数
8. **排序算子** (SortOperator) - ORDER BY 执行
9. **限制算子** (LimitOperator) - LIMIT/OFFSET 执行
10. **DML 执行器** (Insert/Update/Delete) - 数据修改操作
11. **DDL 执行器** (Create/Drop/Alter) - 数据定义操作

---

## Task 1: 表达式求值器 (ExpressionEvaluator)

**Files:**
- Create: `src/executor/ExpressionEvaluator.h`
- Create: `src/executor/ExpressionEvaluator.cpp`
- Test: `tests/test_expression_evaluator.cpp`

**Step 1: 编写测试 - 字面量表达式求值**

```cpp
// tests/test_expression_evaluator.cpp
#include "../src/executor/ExpressionEvaluator.h"
#include "../src/parser/AST.h"
#include "../src/common/Value.h"
#include <cassert>

void testLiteralExpression() {
    using namespace minisql;

    // 创建整数字面量
    auto intLiteral = std::make_shared<parser::LiteralExpr>(Value(42));
    executor::ExpressionEvaluator evaluator;
    auto result = evaluator.evaluate(intLiteral);

    assert(result.isSuccess());
    assert(result.getValue().getInt() == 42);
    assert(result.getValue().getType() == DataType::INT);

    std::cout << "testLiteralExpression: PASSED" << std::endl;
}
```

**Step 2: 运行测试验证失败**

```bash
cd build
cmake --build . --target minisql
./bin/test_expression_evaluator.exe
```

预期: 编译失败 - "ExpressionEvaluator.h: No such file or directory"

**Step 3: 实现 ExpressionEvaluator 基础结构**

```cpp
// src/executor/ExpressionEvaluator.h
#pragma once

#include "../parser/AST.h"
#include "../common/Value.h"
#include "../common/Error.h"
#include "../common/Type.h"
#include "../storage/TableManager.h"
#include <string>
#include <map>
#include <memory>

namespace minisql {
namespace executor {

// 行上下文 (列名 -> 值)
using RowContext = std::map<std::string, Value>;

class ExpressionEvaluator {
public:
    ExpressionEvaluator();

    // 求值表达式
    Result<Value> evaluate(parser::ExprPtr expr);

    // 设置当前行上下文 (用于列引用)
    void setRowContext(const RowContext& context);

private:
    // 字面量求值
    Result<Value> evaluateLiteral(parser::LiteralExpr* expr);

    // 列引用求值
    Result<Value> evaluateColumnRef(parser::ColumnRef* expr);

    // 二元表达式求值
    Result<Value> evaluateBinary(parser::BinaryExpr* expr);

    // 一元表达式求值
    Result<Value> evaluateUnary(parser::UnaryExpr* expr);

    // 函数调用求值
    Result<Value> evaluateFunction(parser::FunctionCall* expr);

    // 算术运算
    Result<Value> evaluateArithmetic(const Value& left, const std::string& op, const Value& right);

    // 逻辑运算
    Result<Value> evaluateLogical(const Value& left, const std::string& op, const Value& right);

    // 比较运算
    Result<Value> evaluateComparison(const Value& left, const std::string& op, const Value& right);

    // 聚合函数求值
    Result<Value> evaluateAggregate(const std::string& funcName, const std::vector<Value>& args);

    // 标量函数求值
    Result<Value> evaluateScalarFunction(const std::string& funcName, const std::vector<Value>& args);

    RowContext rowContext_;
};

} // namespace executor
} // namespace minisql
```

**Step 4: 实现 ExpressionEvaluator.cpp**

```cpp
// src/executor/ExpressionEvaluator.cpp
#include "ExpressionEvaluator.h"
#include "../common/Logger.h"

namespace minisql {
namespace executor {

ExpressionEvaluator::ExpressionEvaluator() {
}

void ExpressionEvaluator::setRowContext(const RowContext& context) {
    rowContext_ = context;
}

Result<Value> ExpressionEvaluator::evaluate(parser::ExprPtr expr) {
    if (!expr) {
        return Result<Value>::err("NULL expression");
    }

    switch (expr->getType()) {
        case ASTNodeType::LITERAL_EXPR:
            return evaluateLiteral(static_cast<parser::LiteralExpr*>(expr.get()));

        case ASTNodeType::COLUMN_REF:
            return evaluateColumnRef(static_cast<parser::ColumnRef*>(expr.get()));

        case ASTNodeType::BINARY_EXPR:
            return evaluateBinary(static_cast<parser::BinaryExpr*>(expr.get()));

        case ASTNodeType::UNARY_EXPR:
            return evaluateUnary(static_cast<parser::UnaryExpr*>(expr.get()));

        case ASTNodeType::FUNCTION_CALL:
            return evaluateFunction(static_cast<parser::FunctionCall*>(expr.get()));

        default:
            return Result<Value>::err("Unknown expression type");
    }
}

Result<Value> ExpressionEvaluator::evaluateLiteral(parser::LiteralExpr* expr) {
    if (!expr) {
        return Result<Value>::err("NULL literal expression");
    }
    return Result<Value>::ok(expr->getValue());
}

Result<Value> ExpressionEvaluator::evaluateColumnRef(parser::ColumnRef* expr) {
    if (!expr) {
        return Result<Value>::err("NULL column reference");
    }

    std::string colName = expr->getColumn();

    // 在行上下文中查找列值
    auto it = rowContext_.find(colName);
    if (it == rowContext_.end()) {
        return Result<Value>::err("Column not found: " + colName);
    }

    return Result<Value>::ok(it->second);
}

Result<Value> ExpressionEvaluator::evaluateBinary(parser::BinaryExpr* expr) {
    if (!expr) {
        return Result<Value>::err("NULL binary expression");
    }

    // 递归求值左右操作数
    auto leftResult = evaluate(expr->getLeft());
    if (!leftResult.isSuccess()) {
        return leftResult;
    }

    auto rightResult = evaluate(expr->getRight());
    if (!rightResult.isSuccess()) {
        return rightResult;
    }

    const Value& left = leftResult.getValue();
    const Value& right = rightResult.getValue();
    const std::string& op = expr->getOp();

    // 根据操作符类型分发
    if (op == "+" || op == "-" || op == "*" || op == "/") {
        return evaluateArithmetic(left, op, right);
    } else if (op == "AND" || op == "OR") {
        return evaluateLogical(left, op, right);
    } else if (op == "=" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") {
        return evaluateComparison(left, op, right);
    }

    return Result<Value>::err("Unknown binary operator: " + op);
}

Result<Value> ExpressionEvaluator::evaluateUnary(parser::UnaryExpr* expr) {
    if (!expr) {
        return Result<Value>::err("NULL unary expression");
    }

    auto operandResult = evaluate(expr->getOperand());
    if (!operandResult.isSuccess()) {
        return operandResult;
    }

    const Value& operand = operandResult.getValue();
    const std::string& op = expr->getOp();

    if (op == "NOT") {
        // 逻辑非
        if (operand.isNull()) {
            return Result<Value>::ok(Value()); // NULL
        }
        bool val = !operand.getBool();
        return Result<Value>::ok(Value(val));
    } else if (op == "-") {
        // 算术负号
        if (operand.isNull()) {
            return Result<Value>::ok(Value()); // NULL
        }
        if (operand.getType() == DataType::INT) {
            return Result<Value>::ok(Value(-operand.getInt()));
        } else if (operand.getType() == DataType::DOUBLE) {
            return Result<Value>::ok(Value(-operand.getDouble()));
        }
    }

    return Result<Value>::err("Unknown unary operator: " + op);
}

Result<Value> ExpressionEvaluator::evaluateFunction(parser::FunctionCall* expr) {
    if (!expr) {
        return Result<Value>::err("NULL function call");
    }

    // 求值所有参数
    std::vector<Value> args;
    for (auto& arg : expr->getArgs()) {
        auto argResult = evaluate(arg);
        if (!argResult.isSuccess()) {
            return argResult;
        }
        args.push_back(argResult.getValue());
    }

    const std::string& funcName = expr->getFuncName();

    // 判断是聚合函数还是标量函数
    if (funcName == "COUNT" || funcName == "SUM" || funcName == "AVG" ||
        funcName == "MAX" || funcName == "MIN") {
        return evaluateAggregate(funcName, args);
    } else {
        return evaluateScalarFunction(funcName, args);
    }
}

Result<Value> ExpressionEvaluator::evaluateArithmetic(const Value& left, const std::string& op, const Value& right) {
    // NULL 处理
    if (left.isNull() || right.isNull()) {
        return Result<Value>::ok(Value()); // NULL
    }

    // 类型提升: 如果任一操作数是 DOUBLE，结果为 DOUBLE
    if (left.getType() == DataType::DOUBLE || right.getType() == DataType::DOUBLE) {
        double l = left.getType() == DataType::INT ? left.getInt() : left.getDouble();
        double r = right.getType() == DataType::INT ? right.getInt() : right.getDouble();
        double result = 0.0;

        if (op == "+") result = l + r;
        else if (op == "-") result = l - r;
        else if (op == "*") result = l * r;
        else if (op == "/") {
            if (r == 0.0) return Result<Value>::err("Division by zero");
            result = l / r;
        }

        return Result<Value>::ok(Value(result));
    } else {
        // 整数运算
        int64_t l = left.getInt();
        int64_t r = right.getInt();
        int64_t result = 0;

        if (op == "+") result = l + r;
        else if (op == "-") result = l - r;
        else if (op == "*") result = l * r;
        else if (op == "/") {
            if (r == 0) return Result<Value>::err("Division by zero");
            result = l / r;
        }

        return Result<Value>::ok(Value(result));
    }
}

Result<Value> ExpressionEvaluator::evaluateLogical(const Value& left, const std::string& op, const Value& right) {
    bool l = left.isNull() ? false : left.getBool();
    bool r = right.isNull() ? false : right.getBool();
    bool result = false;

    if (op == "AND") {
        result = l && r;
    } else if (op == "OR") {
        result = l || r;
    }

    return Result<Value>::ok(Value(result));
}

Result<Value> ExpressionEvaluator::evaluateComparison(const Value& left, const std::string& op, const Value& right) {
    // NULL 处理
    if (left.isNull() || right.isNull()) {
        return Result<Value>::ok(Value()); // NULL
    }

    int cmp = left.compareTo(right);
    bool result = false;

    if (op == "=") result = (cmp == 0);
    else if (op == "!=") result = (cmp != 0);
    else if (op == "<") result = (cmp < 0);
    else if (op == ">") result = (cmp > 0);
    else if (op == "<=") result = (cmp <= 0);
    else if (op == ">=") result = (cmp >= 0);

    return Result<Value>::ok(Value(result));
}

Result<Value> ExpressionEvaluator::evaluateAggregate(const std::string& funcName, const std::vector<Value>& args) {
    // 聚合函数在 AggregateOperator 中处理
    // 这里只处理简单的常量聚合
    return Result<Value>::err("Aggregate functions should be handled by AggregateOperator");
}

Result<Value> ExpressionEvaluator::evaluateScalarFunction(const std::string& funcName, const std::vector<Value>& args) {
    // 字符串函数
    if (funcName == "CONCAT") {
        if (args.size() < 2) return Result<Value>::err("CONCAT requires at least 2 arguments");
        std::string result = "";
        for (const auto& arg : args) {
            if (arg.isNull()) return Result<Value>::ok(Value());
            result += arg.getString();
        }
        return Result<Value>::ok(Value(result));
    }
    else if (funcName == "UPPER") {
        if (args.size() != 1) return Result<Value>::err("UPPER requires exactly 1 argument");
        if (args[0].isNull()) return Result<Value>::ok(Value());
        std::string str = args[0].getString();
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        return Result<Value>::ok(Value(str));
    }
    else if (funcName == "LOWER") {
        if (args.size() != 1) return Result<Value>::err("LOWER requires exactly 1 argument");
        if (args[0].isNull()) return Result<Value>::ok(Value());
        std::string str = args[0].getString();
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return Result<Value>::ok(Value(str));
    }
    // 日期函数
    else if (funcName == "NOW") {
        // 返回当前时间戳
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::string timeStr = std::ctime(&time);
        timeStr.pop_back(); // 移除换行符
        return Result<Value>::ok(Value(timeStr));
    }

    return Result<Value>::err("Unknown scalar function: " + funcName);
}

} // namespace executor
} // namespace minisql
```

**Step 5: 运行测试验证通过**

```bash
cd build
cmake --build . --target minisql
./bin/test_expression_evaluator.exe
```

预期: "testLiteralExpression: PASSED"

**Step 6: 提交**

```bash
git add src/executor/ExpressionEvaluator.h src/executor/ExpressionEvaluator.cpp tests/test_expression_evaluator.cpp
git commit -m "feat(executor): implement ExpressionEvaluator with literal and basic expression evaluation"
```

---

## Task 2: 执行算子基类 (ExecutionOperator)

**Files:**
- Create: `src/executor/ExecutionOperator.h`
- Create: `src/executor/ExecutionOperator.cpp`
- Test: `tests/test_execution_operators.cpp`

**Step 1: 编写测试 - 算子接口**

```cpp
// tests/test_execution_operators.cpp
#include "../src/executor/ExecutionOperator.h"
#include <cassert>

void testOperatorInterface() {
    using namespace minisql::executor;

    // 测试算子接口存在
    // 实际测试将在��体算子实现后进行
    std::cout << "testOperatorInterface: PASSED" << std::endl;
}
```

**Step 2: 运行测试验证失败**

```bash
cd build
cmake --build . --target minisql
./bin/test_execution_operators.exe
```

预期: 编译失败

**Step 3: 实现算子基类**

```cpp
// src/executor/ExecutionOperator.h
#pragma once

#include "../common/Value.h"
#include "../common/Error.h"
#include <vector>
#include <memory>
#include <optional>

namespace minisql {
namespace executor {

// 执行结果元组
using Tuple = std::vector<Value>;

// 执行结果 (带元数据)
struct ExecutionResult {
    std::vector<std::string> columnNames;  // 列名
    std::vector<DataType> columnTypes;      // 列类型
    std::vector<Tuple> rows;                // 数据行

    size_t getRowCount() const { return rows.size(); }
    size_t getColumnCount() const { return columnNames.size(); }
};

// 抽象执行算子 (火山模型迭代器)
class ExecutionOperator {
public:
    virtual ~ExecutionOperator() = default;

    // 初始化算子
    virtual Result<void> open() = 0;

    // 获取下一行 (返回 nullopt 表示没有更多行)
    virtual Result<std::optional<Tuple>> next() = 0;

    // 关闭算子
    virtual Result<void> close() = 0;

    // 获取输出列名
    virtual std::vector<std::string> getColumnNames() const = 0;

    // 获取输出列类型
    virtual std::vector<DataType> getColumnTypes() const = 0;

    // 获取子算子 (如果有)
    virtual std::vector<std::shared_ptr<ExecutionOperator>> getChildren() const {
        return children_;
    }

protected:
    std::vector<std::shared_ptr<ExecutionOperator>> children_;
};

// 执行算子智能指针
using OperatorPtr = std::shared_ptr<ExecutionOperator>;

} // namespace executor
} // namespace minisql
```

**Step 4: 实现 ExecutionOperator.cpp**

```cpp
// src/executor/ExecutionOperator.cpp
#include "ExecutionOperator.h"

namespace minisql {
namespace executor {

// 基类实现为空，具体逻辑由子类实现

} // namespace executor
} // namespace minisql
```

**Step 5: 更新 CMakeLists.txt**

```cmake
# src/executor/CMakeLists.txt
add_library(executor STATIC
    ExecutionOperator.cpp
    ExpressionEvaluator.cpp
    # ... 其他源文件
)

target_include_directories(executor
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(executor
    PUBLIC
        common
        storage
        parser
)
```

**Step 6: 运行测试验证通过**

**Step 7: 提交**

```bash
git add src/executor/ExecutionOperator.h src/executor/ExecutionOperator.cpp src/executor/CMakeLists.txt
git commit -m "feat(executor): implement ExecutionOperator base class with Volcano model iterator interface"
```

---

## Task 3: 表扫描算子 (TableScanOperator)

**Files:**
- Modify: `src/executor/ExecutionOperator.h` (添加 TableScanOperator)
- Modify: `src/executor/ExecutionOperator.cpp`
- Test: `tests/test_table_scan.cpp`

**Step 1: 编写测试 - 全表扫描**

```cpp
// tests/test_table_scan.cpp
#include "../src/executor/ExecutionOperator.h"
#include "../src/storage/TableManager.h"
#include "../src/common/Type.h"
#include <cassert>
#include <iostream>

void testFullTableScan() {
    using namespace minisql;

    // 准备测试数据
    auto& tableMgr = storage::TableManager::getInstance();
    tableMgr.createDatabase("test_db");

    // 创建测试表
    TableDef tableDef;
    tableDef.tableName = "users";
    tableDef.columns.push_back(ColumnDef("id", DataType::INT));
    tableDef.columns.push_back(ColumnDef("name", DataType::VARCHAR, 50));
    tableDef.columns.push_back(ColumnDef("age", DataType::INT));

    tableMgr.createTable("test_db", tableDef);

    // 插入测试数据
    Row row1 = {Value(1), Value("Alice"), Value(25)};
    Row row2 = {Value(2), Value("Bob"), Value(30)};
    tableMgr.insertRow("test_db", "users", row1);
    tableMgr.insertRow("test_db", "users", row2);

    // 创建扫描算子
    auto scanOp = std::make_shared<executor::TableScanOperator>("test_db", "users");

    // 执行扫描
    auto openResult = scanOp->open();
    assert(openResult.isSuccess());

    int rowCount = 0;
    while (true) {
        auto nextResult = scanOp->next();
        assert(nextResult.isSuccess());

        auto rowOpt = nextResult.getValue();
        if (!rowOpt.has_value()) break; // 没有更多行

        rowCount++;
        const auto& row = rowOpt.value();
        assert(row.size() == 3);
    }

    assert(rowCount == 2);

    scanOp->close();

    std::cout << "testFullTableScan: PASSED" << std::endl;
}
```

**Step 2: 运行测试验证失败**

**Step 3: 实现 TableScanOperator**

```cpp
// 在 ExecutionOperator.h 中添加

class TableScanOperator : public ExecutionOperator {
public:
    TableScanOperator(const std::string& dbName, const std::string& tableName);

    Result<void> open() override;
    Result<std::optional<Tuple>> next() override;
    Result<void> close() override;

    std::vector<std::string> getColumnNames() const override;
    std::vector<DataType> getColumnTypes() const override;

private:
    std::string dbName_;
    std::string tableName_;
    TableDef tableDef_;
    storage::TableData tableData_;
    size_t currentRowIndex_;
    bool isOpen_;
};
```

```cpp
// 在 ExecutionOperator.cpp 中实现

TableScanOperator::TableScanOperator(const std::string& dbName, const std::string& tableName)
    : dbName_(dbName), tableName_(tableName), currentRowIndex_(0), isOpen_(false) {
}

Result<void> TableScanOperator::open() {
    auto& tableMgr = storage::TableManager::getInstance();

    // 加载表定义
    auto tableDefResult = tableMgr.getTableDef(dbName_, tableName_);
    if (!tableDefResult.isSuccess()) {
        return Result<void>::err("Table not found: " + tableName_);
    }
    tableDef_ = tableDefResult.getValue();

    // 加载表数据
    auto tableDataResult = tableMgr.loadTable(dbName_, tableName_);
    if (!tableDataResult.isSuccess()) {
        return Result<void>::err("Failed to load table data: " + tableName_);
    }
    tableData_ = tableDataResult.getValue();

    currentRowIndex_ = 0;
    isOpen_ = true;

    LOG_INFO("Opened table scan: " + dbName_ + "." + tableName_);
    return Result<void>::ok();
}

Result<std::optional<Tuple>> TableScanOperator::next() {
    if (!isOpen_) {
        return Result<std::optional<Tuple>>::err("Operator not open");
    }

    if (currentRowIndex_ >= tableData_.size()) {
        return Result<std::optional<Tuple>>::ok(std::nullopt); // EOF
    }

    // 获取当前行
    auto it = tableData_.begin();
    std::advance(it, currentRowIndex_);
    Tuple row = it->second;

    currentRowIndex_++;
    return Result<std::optional<Tuple>>::ok(row);
}

Result<void> TableScanOperator::close() {
    tableData_.clear();
    isOpen_ = false;
    return Result<void>::ok();
}

std::vector<std::string> TableScanOperator::getColumnNames() const {
    std::vector<std::string> names;
    for (const auto& col : tableDef_.columns) {
        names.push_back(col.name);
    }
    return names;
}

std::vector<DataType> TableScanOperator::getColumnTypes() const {
    std::vector<DataType> types;
    for (const auto& col : tableDef_.columns) {
        types.push_back(col.type);
    }
    return types;
}
```

**Step 4-7: 运行测试、提交**

---

## Task 4: 过滤算子 (FilterOperator)

**Files:**
- Modify: `src/executor/ExecutionOperator.h/cpp`
- Test: `tests/test_filter_operator.cpp`

**实现要点:**
- 包装子算子
- 使用 ExpressionEvaluator 求值 WHERE 条件
- 只返回满足条件的行

```cpp
class FilterOperator : public ExecutionOperator {
public:
    FilterOperator(OperatorPtr child, parser::ExprPtr filterExpr);

    Result<void> open() override;
    Result<std::optional<Tuple>> next() override;
    Result<void> close() override;

    std::vector<std::string> getColumnNames() const override;
    std::vector<DataType> getColumnTypes() const override;

private:
    OperatorPtr child_;
    parser::ExprPtr filterExpr_;
    ExpressionEvaluator evaluator_;
    bool isOpen_;
};
```

---

## Task 5: 投影算子 (ProjectOperator)

**Files:**
- Modify: `src/executor/ExecutionOperator.h/cpp`
- Test: `tests/test_project_operator.cpp`

**实现要点:**
- 包装子算子
- 根据 SELECT 列表计算输出列
- 支持表达式计算

```cpp
class ProjectOperator : public ExecutionOperator {
public:
    ProjectOperator(OperatorPtr child, const std::vector<parser::ExprPtr>& projections);

    Result<void> open() override;
    Result<std::optional<Tuple>> next() override;
    Result<void> close() override;

    std::vector<std::string> getColumnNames() const override;
    std::vector<DataType> getColumnTypes() const override;

private:
    OperatorPtr child_;
    std::vector<parser::ExprPtr> projections_;
    ExpressionEvaluator evaluator_;
    bool isOpen_;

    // 推导列名
    std::string deriveColumnName(parser::ExprPtr expr);
};
```

---

## Task 6: 连接算子 (NestedLoopJoinOperator)

**Files:**
- Modify: `src/executor/ExecutionOperator.h/cpp`
- Test: `tests/test_join_operator.cpp`

**实现要点:**
- 双重循环嵌套
- 支持 INNER JOIN 和 LEFT JOIN
- 求值 ON 条件

```cpp
class NestedLoopJoinOperator : public ExecutionOperator {
public:
    enum class JoinType {
        INNER,
        LEFT
    };

    NestedLoopJoinOperator(OperatorPtr left, OperatorPtr right,
                          parser::ExprPtr joinCond, JoinType joinType);

    Result<void> open() override;
    Result<std::optional<Tuple>> next() override;
    Result<void> close() override;

    std::vector<std::string> getColumnNames() const override;
    std::vector<DataType> getColumnTypes() const override;

private:
    OperatorPtr left_;
    OperatorPtr right_;
    parser::ExprPtr joinCond_;
    JoinType joinType_;
    ExpressionEvaluator evaluator_;
    bool isOpen_;

    // 连接状态
    std::optional<Tuple> currentLeftRow_;
    std::vector<Tuple> rightRows_;
    size_t rightRowIndex_;

    Result<void> loadRightTable();
};
```

---

## Task 7: 聚合算子 (AggregateOperator)

**Files:**
- Modify: `src/executor/ExecutionOperator.h/cpp`
- Test: `tests/test_aggregate_operator.cpp`

**实现要点:**
- 支持 GROUP BY
- 支持聚合函数: COUNT, SUM, AVG, MIN, MAX
- 哈希表分组

```cpp
class AggregateOperator : public ExecutionOperator {
public:
    struct AggregateFunc {
        std::string name;  // COUNT, SUM, AVG, MIN, MAX
        parser::ExprPtr arg;
    };

    AggregateOperator(OperatorPtr child,
                     const std::vector<parser::ExprPtr>& groupByExprs,
                     const std::vector<AggregateFunc>& aggregates);

    Result<void> open() override;
    Result<std::optional<Tuple>> next() override;
    Result<void> close() override;

    std::vector<std::string> getColumnNames() const override;
    std::vector<DataType> getColumnTypes() const override;

private:
    OperatorPtr child_;
    std::vector<parser::ExprPtr> groupByExprs_;
    std::vector<AggregateFunc> aggregates_;
    ExpressionEvaluator evaluator_;
    bool isOpen_;

    // 分组状态
    struct GroupKey {
        std::vector<Value> keys;
        bool operator==(const GroupKey& other) const {
            return keys == other.keys;
        }
    };

    struct GroupKeyHash {
        size_t operator()(const GroupKey& k) const {
            size_t h = 0;
            for (const auto& v : k.keys) {
                h ^= std::hash<std::string>{}(v.toString()) + 0x9e3779b9;
            }
            return h;
        }
    };

    struct AggregateState {
        size_t count = 0;
        std::optional<Value> sum;
        std::optional<Value> min;
        std::optional<Value> max;
    };

    std::unordered_map<GroupKey, AggregateState, GroupKeyHash> groups_;
    std::vector<GroupKey> groupOrder_;
    size_t currentGroupIndex_;

    Result<void> buildGroups();
    Result<Value> computeAggregate(const AggregateFunc& func, const AggregateState& state);
};
```

---

## Task 8: 排序算子 (SortOperator)

**Files:**
- Modify: `src/executor/ExecutionOperator.h/cpp`
- Test: `tests/test_sort_operator.cpp`

**实现要点:**
- 物化所有行
- 根据 ORDER BY 表达式排序
- 支持 ASC/DESC

```cpp
class SortOperator : public ExecutionOperator {
public:
    struct SortItem {
        parser::ExprPtr expr;
        bool isAsc;  // true = ASC, false = DESC
    };

    SortOperator(OperatorPtr child, const std::vector<SortItem>& orderBy);

    Result<void> open() override;
    Result<std::optional<Tuple>> next() override;
    Result<void> close() override;

    std::vector<std::string> getColumnNames() const override;
    std::vector<DataType> getColumnTypes() const override;

private:
    OperatorPtr child_;
    std::vector<SortItem> orderBy_;
    ExpressionEvaluator evaluator_;
    bool isOpen_;

    std::vector<Tuple> sortedRows_;
    size_t currentRowIndex_;

    void sortRows();
};
```

---

## Task 9: 限制算子 (LimitOperator)

**Files:**
- Modify: `src/executor/ExecutionOperator.h/cpp`
- Test: `tests/test_limit_operator.cpp**

**实现要点:**
- 支持 LIMIT count
- 支持 OFFSET offset

```cpp
class LimitOperator : public ExecutionOperator {
public:
    LimitOperator(OperatorPtr child, size_t limit, size_t offset = 0);

    Result<void> open() override;
    Result<std::optional<Tuple>> next() override;
    Result<void> close() override;

    std::vector<std::string> getColumnNames() const override;
    std::vector<DataType> getColumnTypes() const override;

private:
    OperatorPtr child_;
    size_t limit_;
    size_t offset_;
    bool isOpen_;
    size_t skipCount_;
    size_t produceCount_;
};
```

---

## Task 10: DML 执行器 (Insert/Update/Delete)

**Files:**
- Modify: `src/executor/DMLExecutor.h/cpp`
- Test: `tests/test_dml_executor.cpp`

**实现要点:**
- Insert: 直接调用 TableManager::insertRow
- Update: 扫描表 -> 求值 SET 表达式 -> 更新行
- Delete: 扫描表 -> 求值 WHERE 条件 -> 删除匹配行

```cpp
class DMLExecutor {
public:
    static Result<ExecutionResult> executeInsert(parser::InsertStmt* stmt);
    static Result<ExecutionResult> executeUpdate(parser::UpdateStmt* stmt);
    static Result<ExecutionResult> executeDelete(parser::DeleteStmt* stmt);
    static Result<ExecutionResult> executeSelect(parser::SelectStmt* stmt);

private:
    static Result<Tuple> evaluateRowValues(const std::vector<parser::ExprPtr>& exprs,
                                         ExpressionEvaluator& evaluator);
};
```

---

## Task 11: DDL 执行器 (Create/Drop/Alter)

**Files:**
- Modify: `src/executor/DDLExecutor.h/cpp`
- Test: `tests/test_ddl_executor.cpp`

**实现要点:**
- Create: 调用 TableManager::createTable
- Drop: 调用 TableManager::dropTable
- Alter: 调用 TableManager 相关方法

```cpp
class DDLExecutor {
public:
    static Result<ExecutionResult> executeCreateTable(parser::CreateTableStmt* stmt);
    static Result<ExecutionResult> executeDropTable(parser::DropStmt* stmt);
    static Result<ExecutionResult> executeAlterTable(parser::AlterTableStmt* stmt);
    static Result<ExecutionResult> executeCreateDatabase(parser::CreateDatabaseStmt* stmt);
    static Result<ExecutionResult> executeDropDatabase(parser::DropDatabaseStmt* stmt);
    static Result<ExecutionResult> executeUse(parser::UseStmt* stmt);
    static Result<ExecutionResult> executeShow(parser::ShowStmt* stmt);
};
```

---

## Task 12: 主执行器 (Executor)

**Files:**
- Modify: `src/executor/Executor.h/cpp`
- Test: `tests/test_executor_integration.cpp`

**实现要点:**
- 生成执行计划
- 构建算子树
- 执行并返回结果

```cpp
class Executor {
public:
    Result<ExecutionResult> execute(std::shared_ptr<parser::Statement> stmt);

private:
    Result<OperatorPtr> buildSelectPlan(parser::SelectStmt* stmt);
    Result<OperatorPtr> buildFromPlan(const std::vector<parser::TableRefPtr>& tables);
};
```

---

## 测试策略

### 单元测试
- 每个算子独立测试
- 使用内存数据库
- 测试边界条件

### 集成测试
- 完整 SQL 语句执行
- 多算子组合
- 真实数据持久化

### 性能测试
- 大数据集扫描
- 连接性能
- 聚合性能

---

## 开发顺序建议

1. **核心基础** (Tasks 1-2): ExpressionEvaluator, ExecutionOperator
2. **数据访问** (Task 3): TableScanOperator
3. **数据操作** (Task 10-11): DML/DDL 执行器
4. **查询处理** (Tasks 4-9): Filter, Project, Join, Aggregate, Sort, Limit
5. **集成** (Task 12): 主执行器

---

## 预估工作量

- **Task 1**: 4-6 小时 (表达式求值)
- **Task 2**: 1-2 小时 (算子基类)
- **Task 3**: 2-3 小时 (表扫描)
- **Task 4**: 2-3 小时 (过滤)
- **Task 5**: 2-3 小时 (投影)
- **Task 6**: 4-6 小时 (连接)
- **Task 7**: 6-8 小时 (聚合)
- **Task 8**: 3-4 小时 (排序)
- **Task 9**: 1-2 小时 (限制)
- **Task 10**: 3-4 小时 (DML)
- **Task 11**: 2-3 小时 (DDL)
- **Task 12**: 4-6 小时 (主执行器)

**总计**: 约 40-60 小时

---

## 技术要点

### 火山模型
- 每个算子实现 next() 接口
- 惰性求值，按需读取
- 支持流水线执行

### 表达式求值
- 递归下降求值
- 类型自动提升
- NULL 传播

### 聚合算法
- 哈希分组
- 单遍扫描
- 内存优化

### 连接算法
- 嵌套循环 (简单但慢)
- 后续可扩展: 哈希连接、归并连接

---

## 依赖关系

```
ExecutionOperator (基类)
    ↓
TableScanOperator → FilterOperator → ProjectOperator
    ↓
NestedLoopJoinOperator
    ↓
AggregateOperator → SortOperator → LimitOperator
    ↓
DML/DDL Executor → 主 Executor
```

---

## 注意事项

1. **错误处理**: 所有操作返回 Result<T>
2. **内存管理**: 使用智能指针管理算子生命周期
3. **类型安全**: 严格检查数据类型
4. **NULL 处理**: 正确处理 NULL 值传播
5. **日志记录**: 关键操作记录日志
6. **测试覆盖**: 每个功能都有对应测试
7. **性能考虑**: 避免不必要的数据拷贝
