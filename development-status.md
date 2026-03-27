# MiniSQL 开发手册

## 项目概述

**项目名称**: MiniSQL - 轻量级 MySQL 风格数据库管理系统
**技术栈**: C++17, Qt 6.5.3, CMake, MinGW
**开发模式**: 模块化、分阶段开发

---

## 开发进度

### ✅ Phase 1: 基础框架 (已完成)

**实现时间**: 2026-03-24

**核心功能**:
- 数据类型系统 (`Type.h/cpp`)
  - 支持所有基本 SQL 数据类型（INT, VARCHAR, DATE, TIMESTAMP 等）
  - 类型转换、比较、验证
- 值系统 (`Value.h/cpp`)
  - 支持 NULL 值
  - SQL 字符串格式化（单引号包裹，`\N` 表示 NULL）
- 错误处理系统 (`Error.h/cpp`)
  - `Result<T>` 模板类用于错误传播
  - 标准 SQL 状态码（SQLSTATE）
- 日志系统 (`Logger.h/cpp`)
  - 分级日志（DEBUG, INFO, WARN, ERROR）
  - 文件日志输出

**测试覆盖**: 57/57 通过

**文件清单**:
```
src/common/
├── Type.h/cpp           # 数据类型定义和工具类
├── Value.h/cpp          # 值封装和操作
├── Error.h/cpp          # 错误处理和结果包装
└── Logger.h/cpp         # 日志系统
```

---

### ✅ Phase 2: 存储与索引引擎 (已完成)

**实现时间**: 2026-03-24

**核心功能**:

#### 2.1 文件操作 (`FileIO.h/cpp`)
- ✅ 目录管理：创建、删除、遍历
- ✅ 文件读写：创建、读取、追加、删除
- ✅ 路径操作：拼接、解析
- ✅ 存储目录结构：`data/dbname/table.{meta,data,idx,rowid}`

#### 2.2 表管理 (`TableManager.h/cpp`)
- ✅ 数据库操作：创建、删除、列表
- ✅ 表操作：创建、删除、存在性检查
- ✅ 数据操作：插入、更新、删除、查询
- ✅ 元数据序列化：JSON 格式表结构
- ✅ 数据持久化：CSV 格式数据文件
- ✅ 自增 ID：`.rowid` 文件管理
- ✅ ALTER TABLE 支持（ADD/DROP COLUMN, RENAME TABLE）

#### 2.3 B+树索引 (`BPlusTree.h/cpp`)
- ✅ 基本操作：插入、查找、删除
- ✅ 范围查询：支持 [min, max] 范围搜索
- ✅ 树分裂：自动分裂和树高度调整
- ✅ 叶子节点链表：支持顺序遍历
- ✅ 可配置阶数
- ✅ 索引持久化：完整 B+ 树序列化/反序列化
- ✅ 重启后索引数据恢复

#### 2.4 索引管理 (`IndexManager.h/cpp`)
- ✅ 索引创建：主键、唯一、普通索引
- ✅ 索引删除
- ✅ 索引查找
- ✅ 运行时索引缓存
- ✅ 索引持久化

**B+树特点**:
- 所有数据在叶子节点
- 叶子节点双向链表连接
- 内部节点只存储索引键
- 支持重复键检测
- 完整序列化支持（前序遍历 + JSON 格式）
- 叶子节点 next 链表恢复

**表结构文件 (.meta) 格式**:
```json
{
    "table_name": "users",
    "engine": "MiniSQL",
    "database": "test_db",
    "columns": [
        {"name": "id", "type": "INT", "primary_key": true},
        {"name": "name", "type": "VARCHAR", "length": 50, "not_null": true}
    ],
    "indexes": [
        {"name": "PRIMARY", "columns": ["id"], "type": "BTREE", "unique": true}
    ],
    "foreign_keys": []
}
```

**数据文件 (.data) 格式**:
```
1,'Alice',25
2,'Bob',\N
```

**测试覆盖**: 1134/1134 通过

**文件清单**:
```
src/storage/
├── FileIO.h/cpp         # 文件操作
├── TableManager.h/cpp   # 表管理
├── BPlusTree.h/cpp      # B+树索引
└── IndexManager.h/cpp   # 索引管理器
```

---

### ✅ Phase 3: SQL 解析器 (已完成)

**实现时间**: 2026-03-26

**核心功能**:

#### 3.1 词法分析器 (`Lexer.h/cpp`)
- ✅ Token 类型定义（关键字、标识符、字面量、操作符等）
- ✅ SQL 关键字识别（100+ 关键字，大小写不敏感）
- ✅ 数字解析（整数、浮点数、科学计数法）
- ✅ 字符串解析（单引号、双引号、转义字符）
- ✅ 标识符解析
- ✅ 操作符解析（=, <, >, <=, >=, <>, != 等）
- ✅ 注释处理（单行 `--`、多行 `/* */`）
- ✅ peekToken 预读支持

#### 3.2 AST 节点定义 (`AST.h`)
- ✅ 基础节点类 (`ASTNode`)
- ✅ 表达式节点：`LiteralExpr`, `ColumnRef`, `BinaryExpr`, `UnaryExpr`
- ✅ 语句节点：`SelectStmt`, `InsertStmt`, `UpdateStmt`, `DeleteStmt`, `CreateTableStmt`, `DropStmt`, `UseStmt`, `ShowStmt`, `AlterTableStmt`
- ✅ 表引用节点：`TableRef`, `JoinClause`
- ✅ 列定义节点：`ColumnDefNode`, `ConstraintDefNode`

#### 3.3 语法分析器 (`Parser.h/cpp`)
- ✅ USE 语句：`USE database;`
- ✅ SHOW 语句：`SHOW DATABASES;`, `SHOW TABLES;`
- ✅ SELECT 语句：
  - SELECT 列表、DISTINCT
  - FROM 子句（含别名解析修复）
  - WHERE 子句
  - JOIN 子句（INNER, LEFT, RIGHT, FULL, CROSS）
  - GROUP BY、HAVING
  - ORDER BY、LIMIT、OFFSET
- ✅ INSERT 语句：`INSERT INTO table (cols) VALUES (...);`
- ✅ UPDATE 语句：`UPDATE table SET col=val WHERE ...;`
- ✅ DELETE 语句：`DELETE FROM table WHERE ...;`
- ✅ CREATE TABLE 语句（支持 IF NOT EXISTS、列约束）
- ✅ DROP TABLE 语句（支持 IF EXISTS）
- ✅ ALTER TABLE 语句：
  - ADD COLUMN / ADD (添加列)
  - DROP COLUMN / DROP (删除列)
  - MODIFY COLUMN (修改列类型)
  - CHANGE COLUMN (重命名列并修改类型)
  - RENAME COLUMN (重命名列)
  - RENAME TO (重命名表)
  - ADD CONSTRAINT / DROP CONSTRAINT (约束操作)
- ✅ 函数调用表达式：
  - COUNT, SUM, AVG, MIN, MAX (聚合函数)
  - CONCAT, SUBSTRING, UPPER, LOWER (字符串函数)
  - NOW, DATE, YEAR, MONTH, DAY (日期函数)
  - 嵌套函数调用
- ✅ 表达式解析（递归下降 + Pratt Parser 技术处理优先级）

#### 3.4 语义分析器 (`SemanticAnalyzer.h/cpp`)
- ✅ 表存在性检查
- ✅ 字段存在性检查
- ✅ 数据类型验证
- ✅ 约束检查（重复列名、空列名、表已存在等）
- ✅ 表达式验证（列引用检查）
- ✅ ALTER TABLE 语句语义检查
- ✅ 函数调用参数验证

**测试覆盖**: 188/188 通过

**文件清单**:
```
src/parser/
├── Token.h/cpp           # Token 类型和关键字表
├── Lexer.h/cpp           # 词法分析器
├── AST.h                 # AST 节点定义
├── Parser.h/cpp          # 语法分析器
└── SemanticAnalyzer.h/cpp # 语义分析器

tests/
├── test_lexer.cpp        # 122 测试
├── test_ast.cpp          # 10 测试
├── test_parser.cpp       # 40 测试
└── test_semantic.cpp     # 16 测试
```

---

### ✅ Phase 4: 查询执行器 (已完成)

**实现时间**: 2026-03-26 ~ 2026-03-27

**核心功能**:

#### 4.1 算子体系 (Volcano 模型)
基于迭代器模式，所有算子实现统一的 `open()`、`next()`、`close()` 接口：

```
ExecutionOperator (抽象基类)
├── TableScanOperator    # 表扫描
├── FilterOperator       # 条件过滤
├── ProjectOperator      # 列投影
├── NestedLoopJoinOperator # 嵌套循环连接
├── AggregateOperator    # 聚合计算
├── SortOperator         # 排序
└── LimitOperator        # 限制返回行数
```

#### 4.2 表达式求值器 (`ExpressionEvaluator.h/cpp`)
- ✅ 行上下文 (`RowContext`) 管理
- ✅ 字面量求值（INT, VARCHAR, FLOAT, NULL）
- ✅ 列引用求值
- ✅ 算术运算（+, -, *, /, %）
- ✅ 比较运算（=, <>, <, >, <=, >=）
- ✅ 逻辑运算（AND, OR, NOT）
- ✅ 函数调用（COUNT, SUM, AVG, MIN, MAX, CONCAT, UPPER, LOWER 等）

#### 4.3 执行算子

**TableScanOperator** (`TableScanOperator.h/cpp`)
- ✅ 全表扫描
- ✅ 支持指定数据库和表名
- ✅ 返回完整行数据

**FilterOperator** (`FilterOperator.h/cpp`)
- ✅ 从子算子逐行获取数据
- ✅ 使用表达式求值器计算过滤条件
- ✅ 仅返回满足条件的行

**ProjectOperator** (`ProjectOperator.h/cpp`)
- ✅ SELECT 列投影
- ✅ 支持表达式投影（如 `SELECT id + 1`）
- ✅ 处理聚合函数结果列

**NestedLoopJoinOperator** (`NestedLoopJoinOperator.h/cpp`)
- ✅ 嵌套循环连接算法
- ✅ 支持 ON 条件连接
- ✅ 支持 LEFT/RIGHT/INNER/CROSS JOIN

**AggregateOperator** (`AggregateOperator.h/cpp`)
- ✅ GROUP BY 分组
- ✅ 聚合函数：COUNT, SUM, AVG, MIN, MAX
- ✅ 支持 HAVING 子句

**SortOperator** (`SortOperator.h/cpp`)
- ✅ ORDER BY 排序
- ✅ 支持升序/降序
- ✅ 支持多字段排序
- ✅ 支持表达式排序键

**LimitOperator** (`LimitOperator.h/cpp`)
- ✅ LIMIT 限制返回行数
- ✅ OFFSET 跳过指定行数
- ✅ 支持 LIMIT/OFFSET 组合

#### 4.4 DML 执行器 (`DMLExecutor.h/cpp`)
- ✅ INSERT：插入单行或多行数据
- ✅ UPDATE：更新满足条件的行（含 WHERE 子句）
- ✅ DELETE：删除满足条件的行（含 WHERE 子句）

#### 4.5 DDL 执行器 (`DDLExecutor.h/cpp`)
- ✅ CREATE DATABASE：创建新数据库
- ✅ CREATE TABLE：创建表（含列定义和约束）
- ✅ DROP DATABASE：删除数据库
- ✅ DROP TABLE：删除表
- ✅ ALTER TABLE：表结构变更
  - ADD COLUMN / DROP COLUMN
  - RENAME TABLE
- ✅ USE DATABASE：切换当前数据库

#### 4.6 执行计划构建 (`Executor.h/cpp`)
- ✅ SELECT 语句执行流程编排
- ✅ 算子树构建（从底向上）
- ✅ 聚合函数提取与处理
- ✅ 执行结果收集

**执行计划示例**:
```
SELECT name, COUNT(*) FROM users WHERE age > 18 GROUP BY name ORDER BY name LIMIT 10;

执行计划:
LimitOperator (LIMIT 10)
└── ProjectOperator (name, COUNT(*))
    └── SortOperator (ORDER BY name)
        └── AggregateOperator (GROUP BY name, COUNT(*))
            └── FilterOperator (WHERE age > 18)
                └── TableScanOperator (users)
```

**测试覆盖**: 16 个测试套件全部通过

**测试清单**:
```
tests/
├── test_expression_evaluator.cpp  # 表达式求值
├── test_table_scan.cpp            # 表扫描算子
├── test_filter_operator.cpp        # 过滤算子
├── test_project_operator.cpp      # 投影算子
├── test_join_operator.cpp          # 连接算子
├── test_limit_operator.cpp         # 限制算子
├── test_sort_operator.cpp          # 排序算子
├── test_aggregate_operator.cpp     # 聚合算子
├── test_dml_executor.cpp           # DML执行器
├── test_ddl_executor.cpp           # DDL执行器
└── test_executor.cpp               # 执行器集成测试
```

**新增文件清单**:
```
src/executor/
├── ExecutionOperator.h/cpp     # 算子基类
├── ExpressionEvaluator.h/cpp   # 表达式求值器
├── TableScanOperator.h/cpp     # 表扫描
├── FilterOperator.h/cpp        # 过滤
├── ProjectOperator.h/cpp       # 投影
├── NestedLoopJoinOperator.h/cpp # 连接
├── AggregateOperator.h/cpp     # 聚合
├── SortOperator.h/cpp          # 排序
├── LimitOperator.h/cpp         # 限制
├── DMLExecutor.h/cpp           # DML执行器
├── DDLExecutor.h/cpp           # DDL执行器
└── Executor.h/cpp              # 执行器入口
```

---

## Phase 2 ~ 4 未实现功能

### ❌ 视图支持

**设计要求**: `view1.view` 文件
**状态**: 未开始
**需要模块**: ViewManager

---

### ❌ 存储过程支持

**设计要求**: `proc1.proc` 文件
**状态**: 未开始
**需要模块**: ProcedureManager

---

### ❌ 触发器支持

**设计要求**: `trigger1.trigger` 文件
**状态**: 未开始
**需要模块**: TriggerManager

---

## Phase 3 未实现功能

### ⚠️ 子查询支持

**当前状态**: 未实现
**需要功能**:
- IN 子查询
- EXISTS 子查询
- 标量子查询
- 关联子查询

**预估工作量**: 100-150 行代码

---

### ⚠️ 语义分析增强

**当前状态**: 基础实现
**需要功能**:
- INSERT/UPDATE 时数据类型验证（值类型 vs 列定义类型）
- 非空约束验证
- 主键/唯一约束检查
- 外键约束检查
- 默认值处理

**预估工作量**: 100-200 行代码

---

## 文件格式规范

### .meta 文件 (表结构)
```json
{
    "table_name": "表名",
    "engine": "MiniSQL",
    "database": "数据库名",
    "comment": "表注释",
    "columns": [
        {
            "name": "列名",
            "type": "数据类型",
            "length": 长度,           // VARCHAR/CHAR
            "precision": 精度,        // DECIMAL
            "scale": 小数位,          // DECIMAL
            "not_null": false,
            "primary_key": false,
            "unique": false,
            "auto_increment": false,
            "has_default": false,
            "default_value": "默认值",
            "comment": "列注释"
        }
    ],
    "indexes": [
        {
            "name": "索引名",
            "table_name": "表名",
            "columns": ["列1", "列2"], // 支持复合索引
            "type": "BTREE",          // BTREE 或 HASH
            "unique": false
        }
    ],
    "foreign_keys": [
        {
            "name": "外键名",
            "column": "当前表列",
            "ref_table": "引用表",
            "ref_column": "引用列",
            "on_delete": "CASCADE",   // CASCADE, SET NULL, RESTRICT, NO ACTION
            "on_update": "CASCADE"
        }
    ]
}
```

### .data 文件 (表数据)
```
格式: CSV 风格
规则:
- 数字直接存储: 123, 45.67
- 字符串用单引号: 'Hello World', 'Tom''s car'
- NULL 用 \N 表示: \N
- 字段用逗号分隔

示例:
1,'Alice',25,'alice@example.com'
2,'Bob',\N,'bob@example.com'
3,'Charlie',30,'charlie@test.com'
```

### .idx 文件 (索引)
**实现状态**: ✅ 已完整实现
**格式**: JSON 格式序列化的 B+ 树结构
**内容**:
- 树的阶数 (order)
- 所有节点的键值对
- 节点间的父子关系
- 叶子节点的 next 链表指针
- 支持完整的树结构恢复

**示例结构**:
```json
{
  "order": 200,
  "version": 1,
  "rootIndex": 0,
  "nodes": [
    "{\"isLeaf\":false,\"parentIndex\":-1,\"childIndex\":0,\"keys\":[50,100],\"childCount\":3}",
    "{\"isLeaf\":true,\"parentIndex\":0,\"childIndex\":0,\"keys\":[10,20,30],\"values\":[1,2,3],\"selfIndex\":1}",
    "{\"isLeaf\":true,\"parentIndex\":0,\"childIndex\":1,\"keys\":[50,60,70],\"values\":[4,5,6],\"selfIndex\":2}",
    "{\"isLeaf\":true,\"parentIndex\":0,\"childIndex\":2,\"keys\":[100,150],\"values\":[7,8],\"selfIndex\":3}"
  ]
}
```

### .rowid 文件 (自增ID)
```
内容: 下一个可用的自增ID
示例: 3
```

---

## 测试运行

### 编译项目
```bash
cd build
cmake ../src
cmake --build . --target executor
```

### 运行测试
```bash
# 公共模块测试
./bin/test_common.exe

# 存储模块测试
./bin/test_storage.exe

# 执行器测试
./bin/test_executor.exe
./bin/test_expression_evaluator.exe
./bin/test_table_scan.exe
./bin/test_filter_operator.exe
./bin/test_project_operator.exe
./bin/test_join_operator.exe
./bin/test_limit_operator.exe
./bin/test_sort_operator.exe
./bin/test_aggregate_operator.exe
./bin/test_dml_executor.exe
./bin/test_ddl_executor.exe
```

### 测试结果
- **test_common**: 60/60 通过
- **test_storage**: 1134/1134 通过
- **test_lexer**: 122/122 通过
- **test_ast**: 10/10 通过
- **test_parser**: 57/57 通过
- **test_semantic**: 16/16 通过
- **test_expression_evaluator**: 通过
- **test_table_scan**: 通过
- **test_filter_operator**: 通过
- **test_project_operator**: 通过
- **test_join_operator**: 通过
- **test_limit_operator**: 通过
- **test_sort_operator**: 通过
- **test_aggregate_operator**: 通过
- **test_dml_executor**: 通过
- **test_ddl_executor**: 通过
- **test_executor**: 通过

**累计测试**: 1400+ 测试通过

---

## 目录结构

```
E:\pro\SQL_Management_System\
├── src/
│   ├── common/          # 公共模块 (Phase 1)
│   │   ├── Type.h/cpp
│   │   ├── Value.h/cpp
│   │   ├── Error.h/cpp
│   │   └── Logger.h/cpp
│   │
│   ├── storage/         # 存储引擎 (Phase 2)
│   │   ├── FileIO.h/cpp
│   │   ├── TableManager.h/cpp
│   │   ├── BPlusTree.h/cpp
│   │   └── IndexManager.h/cpp
│   │
│   ├── parser/          # SQL 解析器 (Phase 3)
│   │   ├── Token.h/cpp
│   │   ├── Keywords.cpp
│   │   ├── Lexer.h/cpp
│   │   ├── AST.h
│   │   ├── Parser.h/cpp
│   │   └── SemanticAnalyzer.h/cpp
│   │
│   ├── executor/        # 查询执行器 (Phase 4)
│   │   ├── ExecutionOperator.h/cpp
│   │   ├── ExpressionEvaluator.h/cpp
│   │   ├── TableScanOperator.h/cpp
│   │   ├── FilterOperator.h/cpp
│   │   ├── ProjectOperator.h/cpp
│   │   ├── NestedLoopJoinOperator.h/cpp
│   │   ├── AggregateOperator.h/cpp
│   │   ├── SortOperator.h/cpp
│   │   ├── LimitOperator.h/cpp
│   │   ├── DMLExecutor.h/cpp
│   │   ├── DDLExecutor.h/cpp
│   │   └── Executor.h/cpp
│   │
│   └── external/        # 外部依赖
│       └── json.hpp     # nlohmann/json 库
│
├── tests/               # 测试代码
│   ├── test_utils.h/cpp
│   ├── test_common.cpp
│   ├── test_storage.cpp
│   ├── test_lexer.cpp
│   ├── test_ast.cpp
│   ├── test_parser.cpp
│   ├── test_semantic.cpp
│   ├── test_expression_evaluator.cpp
│   ├── test_table_scan.cpp
│   ├── test_filter_operator.cpp
│   ├── test_project_operator.cpp
│   ├── test_join_operator.cpp
│   ├── test_limit_operator.cpp
│   ├── test_sort_operator.cpp
│   ├── test_aggregate_operator.cpp
│   ├── test_dml_executor.cpp
│   ├── test_ddl_executor.cpp
│   └── test_executor.cpp
│
├── build/               # 编译输出
│   └── bin/
│       ├── data/        # 数据存储目录
│       └── logs/        # 日志目录
│
└── docs/                # 文档
```

---

## 数据存储示例

运行 `demo_file_creation.exe` 后生成的文件：

```
data/demo_db/
├── users.meta           # 表结构定义 (JSON)
├── users.data           # 表数据 (CSV)
├── users.rowid          # 自增ID计数器
└── idx_name.idx         # 索引文件
```

---

## 已知限制

1. **外键约束**: 定义已支持，但未实现约束检查
2. **事务支持**: 未实现
3. **并发控制**: 未实现
4. **视图**: 未实现
5. **存储过程**: 未实现
6. **触发器**: 未实现
7. **子查询**: 未实现

---

## 下一阶段规划

### Phase 5: 事务管理 (未开始)
- ACID 特性
- 并发控制
- 日志与恢复

### Phase 6: 优化器 (待规划)
- 查询重写
- 代价估算
- 连接顺序优化

---

## 技术债务

### 高优先级
- [ ] 外键约束检查
- [ ] 数据类型验证（插入时）
- [ ] 子查询支持

### 中优先级
- [ ] B+树删除算法完善
- [ ] 复合索引的完整支持
- [ ] 索引优化器

### 低优先级
- [ ] 视图实现
- [ ] 存储过程实现
- [ ] 触发器实现

---

## 开发规范

### 代码风格
- 使用 C++17 标准
- 类名大驼峰: `TableManager`
- 方法名小驼峰: `createTable`
- 成员变量下划线后缀: `order_`
- 常量大写下划线: `MAX_KEYS`

### 错误处理
- 使用 `Result<T>` 返回错误
- 错误消息包含上下文信息
- 记录 ERROR 级别日志

### 测试规范
- 每个模块对应测试文件
- 测试用例命名: `test_<module>_<feature>`
- 使用自定义测试宏: `ASSERT_TRUE`, `ASSERT_EQ` 等
- 测试文件不自动退出（等待用户输入）

---

## 提交记录

- **2e7e544**: feat: 完成第一阶段 - 基础框架
- **82743b3**: feat: 实现第二阶段 - 存储与索引引擎
- **ceaeb95**: feat(parser): 实现完整的 SQL 解析器（Lexer、AST、Parser、SemanticAnalyzer）
- **1393cfa**: feat(storage): 实现 B+ 树索引持久化
- **00b9b88**: feat(parser): 实现 ALTER TABLE 和函数调用表达式
- **(本次)**: feat(executor): 实现查询执行器（Volcano 模型、全部执行算子、DML/DDL 执行器）

---

## 联系方式

- **GitHub**: https://github.com/chenzhou0071/SQL_Management_System
- **分支**: main

---

*最后更新: 2026-03-27*
