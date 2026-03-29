# MiniSQL 开发手册

## 项目概述

**项目名称**: MiniSQL - 轻量级 MySQL 风格数据库管理系统
**技术栈**: C++17, Qt 6.5.3, CMake, MinGW
**开发模式**: 模块化、分阶段开发

---

## 开发进度

### ✅ Phase 1: 基础框架

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

### ✅ Phase 2: 存储与索引引擎

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

### ✅ Phase 3: SQL 解析器

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

#### 3.4 子查询支持 (`Parser.cpp`, `AST.h`)
- ✅ IN 子查询：`WHERE id IN (SELECT ...)`
- ✅ NOT IN 子查询：`WHERE id NOT IN (SELECT ...)`
- ✅ EXISTS 子查询：`WHERE EXISTS (SELECT ...)`
- ✅ NOT EXISTS 子查询：`WHERE NOT EXISTS (SELECT ...)`
- ✅ AST 节点：`InSubqueryExpr`, `ExistsExpr`, `SubqueryExpr`
- ✅ `AnalyzeStmt` 节点用于统计信息收集
- ✅ `ExplainStmt` 节点用于执行计划解释

#### 3.5 语义分析器 (`SemanticAnalyzer.h/cpp`)
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
├── test_semantic.cpp     # 16 测试
└── test_subquery.cpp     # 子查询测试
```

---

---

### ✅ Phase 4: 查询执行器

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

### ✅ Phase 5: 查询优化器

**实现时间**: 2026-03-27 ~ 2026-03-28

**核心功能**:

#### 5.1 统计信息管理 (`Statistics.h/cpp`)
- ✅ 表统计信息收集（行数、表大小、最后分析时间）
- ✅ 列统计信息收集（distinct 值、NULL 数量、平均长度、min/max）
- ✅ 统计信息持久化（`data/{db}/{table}.stats` JSON 文件）
- ✅ 内存缓存机制
- ✅ Selectivity 估算
- ✅ 行数估算

#### 5.2 索引选择器 (`IndexSelector.h/cpp`)
- ✅ 获取表上所有可用索引
- ✅ 从 WHERE 条件提取列引用
- ✅ 判断等值/范围条件
- ✅ 计算索引选择性（selectivity）
- ✅ 判断是否覆盖索引（covering index）
- ✅ 选择最优索引

#### 5.3 查询重写器 (`QueryRewriter.h/cpp`)
- ✅ 常量折叠（`age > 10 + 5` → `age > 15`）
- ✅ 消除冗余条件（`age > 18 OR age = 20` → `age > 18`）
- ✅ 简化布尔表达式（`AND true` → 直接返回、`OR false` → 消除）

#### 5.4 规则优化器 (`RuleOptimizer.h/cpp`)
- ✅ 索引使用规则（index_usage）
- ✅ 连接顺序规则（join_order）
- ✅ 排序优化规则（sort_optimization）
- ✅ 聚合优化规则（aggregate_optimization）
- ✅ 投影优化规则（projection_optimization）
- ✅ 可扩展的规则注册机制

#### 5.5 代价优化器 (`CostOptimizer.h/cpp`)
- ✅ 代价模型常量定义（CPU/IO 成本系数）
- ✅ 全表扫描代价估算
- ✅ 索引扫描代价估算
- ✅ 过滤代价估算
- ✅ 排序代价估算
- ✅ 聚合代价估算

#### 5.6 执行计划生成器 (`PlanGenerator.h/cpp`)
- ✅ 逻辑计划生成（PlanNode 树）
  - TableScan 节点
  - Filter 节点
  - Join 节点（NestedLoopJoin）
  - Aggregate 节点
  - Sort 节点
  - Project 节点
  - Limit 节点
- ✅ 物理计划物化（转换为 ExecutionOperator）
- ✅ 索引选择集成
- ✅ 统计信息集成

#### 5.7 EXPLAIN 处理器 (`ExplainHandler.h/cpp`)
- ✅ EXPLAIN 语句解析（`EXPLAIN SELECT ...`）
- ✅ EXPLAIN FORMAT JSON（JSON 格式输出）
- ✅ 表格格式输出
- ✅ JSON 格式输出
- ✅ 访问类型映射（ALL, index, range, ref, eq_ref, const）
- ✅ Extra 信息（Using index, Using filesort, Using temporary）

#### 5.8 执行计划定义 (`ExecutionPlan.h`)
- ✅ ScanType 枚举（FULL_SCAN, INDEX_SCAN, COVERING_SCAN）
- ✅ PlanNode 结构体（节点ID、类型、表名、索引、条件、代价、行数等）

#### 5.9 ANALYZE TABLE 支持
- ✅ `AST.h` 添加 `AnalyzeStmt` 节点
- ✅ `Parser.cpp` 解析 `ANALYZE TABLE table_name`
- ✅ `Executor.cpp` 集成统计信息收集
- ✅ 为 CBO 提供统计信息

#### 5.10 集成到执行器
- ✅ `Parser.cpp` 支持 EXPLAIN 语句
- ✅ `Parser.cpp` 支持 ANALYZE TABLE 语句
- ✅ `Executor.cpp` 执行 EXPLAIN 输出
- ✅ `executor/CMakeLists.txt` 链接 optimizer 模块

**统计信息文件 (.stats) 格式**:
```json
{
  "tableName": "users",
  "rowCount": 1000,
  "tableSize": 51200,
  "lastAnalyzed": "2026-03-28 01:00:00",
  "columns": [
    {
      "columnName": "id",
      "distinctValues": 1000,
      "nullCount": 0,
      "avgLength": 4,
      "minValue": "1",
      "maxValue": "1000"
    }
  ]
}
```

**执行计划示例**:
```
EXPLAIN SELECT * FROM users WHERE age > 18 ORDER BY age;

执行计划:
id=1: Project
  -> id=2: Sort
    -> id=3: TableScan on users using idx_age
```

**测试覆盖**: 9 个优化器模块测试全部通过

**测试清单**:
```
tests/
├── test_statistics.cpp     # 统计信息测试
├── test_index_selector.cpp # 索引选择测试
├── test_query_rewriter.cpp # 查询重写测试
├── test_rule_optimizer.cpp # 规则优化测试
├── test_cost_optimizer.cpp # 代价优化测试
├── test_plan_generator.cpp # 计划生成测试
├── test_explain.cpp       # EXPLAIN 测试
├── test_subquery.cpp      # 子查询解析测试
├── test_integration.cpp   # 集成测试
└── test_foreign_key.cpp   # 外键约束测试
```

**新增文件清单**:
```
src/optimizer/
├── ExecutionPlan.h          # 执行计划定义
├── Statistics.h/cpp          # 统计信息管理
├── IndexSelector.h/cpp       # 索引选择器
├── QueryRewriter.h/cpp       # 查询重写器
├── RuleOptimizer.h/cpp       # 规则优化器
├── CostOptimizer.h/cpp       # 代价优化器
├── PlanGenerator.h/cpp       # 执行计划生成器
└── ExplainHandler.h/cpp      # EXPLAIN 处理器
```

---

### ✅ Phase 6: 基础功能集成

**实现时间**: 2026-03-28

**核心功能**:

#### 6.1 优化器统一入口 (`QueryOptimizer.h/cpp`)
- ✅ 统一调度 QueryRewriter → RuleOptimizer → CostOptimizer → PlanGenerator
- ✅ 单例模式管理
- ✅ 完整的优化流程日志

#### 6.2 执行器集成
- ✅ `Executor::executeSelect()` 使用 QueryOptimizer
- ✅ 完整优化流程：查询重写 → 逻辑计划 → 物理计划 → 规则优化 → 代价优化
- ✅ WHERE 条件正确处理（Filter 节点）

#### 6.3 EXPLAIN/ANALYZE 支持
- ✅ 关键字注册（EXPLAIN, ANALYZE）
- ✅ EXPLAIN 语句执行
- ✅ ANALYZE TABLE 统计信息收集

#### 6.4 集成测试
- ✅ 完整 SQL 执行流程测试
- ✅ Parser → Optimizer → Executor 集成验证

#### 6.5 子查询执行支持
- ✅ IN 子查询执行 (`WHERE id IN (SELECT ...)`)
- ✅ NOT IN 子查询执行 (`WHERE id NOT IN (SELECT ...)`)
- ✅ 子查询表达式求值 (`InSubqueryExpr`, `ExistsExpr`)
- ✅ `ExpressionEvaluator` 子查询执行方法
- ✅ `QueryRewriter` 子查询表达式处理

#### 6.6 外键约束检查
- ✅ 外键解析 (`Parser.cpp`): 支持表级 `FOREIGN KEY (col) REFERENCES tbl(col)` 语法
- ✅ 外键元数据序列化 (`TableManager.cpp`): 存储到 `.meta` JSON 文件
- ✅ 外键元数据反序列化: 从 `.meta` 文件恢复外键定义
- ✅ INSERT 外键约束检查 (`DMLExecutor::executeInsert`)
  - 检查插入值是否在被引用表中存在
  - 外键列为 NULL 时跳过检查
- ✅ UPDATE 外键约束检查 (`DMLExecutor::executeUpdate`)
  - 更新前后均验证外键约束
- ✅ `DDLExecutor::executeCreateTable` 外键复制到 `TableDef`
- ✅ 错误消息: `Foreign key constraint violation: {table}.{column} references {ref_table}.{ref_column} but value (...) does not exist in parent table`

**测试覆盖**: 4/4 外键测试通过
- `testForeignKeyViolation`: 插入不存在的引用值 → 正确拒绝
- `testForeignKeyValid`: 插入有效引用值 → 正确接受
- `testForeignKeyUpdateViolation`: 更新为无效引用值 → 正确拒绝
- `testNoForeignKeyTable`: 无外键表插入 → 正常工作

**执行流程**:
```
SQL 文本
    ↓
Parser (词法→语法→AST)
    ↓
QueryOptimizer
    ├── QueryRewriter (谓词下推、常量折叠)
    ├── PlanGenerator (生成逻辑计划)
    ├── RuleOptimizer (规则优化)
    └── CostOptimizer (代价优化)
    ↓
Executor (物理执行算子)
    ↓
结果
```

**测试覆盖**: 27/27 测试通过

**新增文件清单**:
```
src/optimizer/
└── QueryOptimizer.h/cpp      # 统一优化入口

tests/
└── test_integration.cpp     # 集成测试
```

---

### ✅ Phase 7: 高级 SELECT 支持

**实现时间**: 2026-03-28

**核心功能**:

#### 7.1 JOIN 执行计划生成
- ✅ `PlanGenerator` 遍历 `selectStmt->joins` 生成 NestedLoopJoin 节点
- ✅ `NestedLoopJoinOperator` 实现 INNER/LEFT/RIGHT JOIN
- ✅ 多表连接：遍历 JOIN 列表逐个串联，自动支持三表及以上

#### 7.2 FROM 子查询支持
- ✅ `TableRef` 结构增加 `subquery` 字段
- ✅ `Parser::parseTableRef()` 识别 `(` 开始子查询
- ✅ `SubqueryScanOperator` 封装子计划并递归执行
- ✅ `SELECT * FROM (SELECT ...) AS t` 正常工作

#### 7.3 HAVING 子句完善
- ✅ `AggregateOperator` 正确传递 `havingClause` 给 `AggregateOperator`
- ✅ `PlanGenerator::generateAggregateNode()` 接收并传递 HAVING 条件

#### 7.4 关键 Bug 修复
- ✅ **Result<T> 悬空指针修复**：所有 `const T& val = *result.getValue()` 改为 `Value val = *result.getValue()`，避免 `~Result()` 销毁堆内存后引用悬空。涉及文件：
  - `AggregateOperator.cpp` (`buildGroups`, `next`)
  - `FilterOperator.cpp`
  - `NestedLoopJoinOperator.cpp`
  - `SortOperator.cpp`
  - `DMLExecutor.cpp` (两处 WHERE 条件)
  - `ExpressionEvaluator.cpp` (多处二元/一元/IN 子查询表达式)
- ✅ **ProjectOperator 聚合函数崩溃修复**：`ExpressionEvaluator::evaluateAggregate` 对 `COUNT/SUM/AVG` 返回错误（因为聚合已在 AggregateOperator 预计算）。修复：ProjectOperator 检测聚合函数投影时，直接从子算子输出的 GROUP BY 列之后位置取值，避免重复求值
- ✅ **HAVING 条件重新启用**：修复悬空指针后，`AggregateOperator::next()` 中的 HAVING 过滤逻辑恢复正常
- ✅ **索引选择恢复**：`PlanGenerator::generateScanNode()` 中被注释的索引选择代码恢复，并修复指针解引用（`IndexSelector` 结果正确传递 `indexName` 和 `scanType`）

**测试覆盖**:
- `test_join`: INNER/LEFT/RIGHT JOIN，多表连接 — 全部通过
- `test_from_subquery`: FROM 子查询，FROM 子查询 + JOIN — 全部通过
- `test_having`: GROUP BY，GROUP BY + HAVING — 全部通过

**新增文件清单**:
```
src/executor/
├── SubqueryScanOperator.h/cpp  # FROM 子查询扫描算子

tests/
├── test_join.cpp              # JOIN 执行测试
├── test_from_subquery.cpp     # FROM 子查询测试
└── test_having.cpp           # HAVING 子句测试
```

---

### ✅ Phase 8: Linux 服务器实现

**实现时间**: 2026-03-29

**核心功能**:

#### 8.1 服务器架构设计
- ✅ **Single Reactor + Thread Pool 模型**
  - 主 Reactor 线程池（2个线程）处理 I/O 事件
  - 工作线程池（4个线程）处理 SQL 执行
  - epoll 事件驱动，非阻塞 I/O
  - 支持多客户端并发连接

#### 8.2 并发控制层 (`src/server/concurrency/`)
- ✅ **RWLock** (`RWLock.h/cpp`)
  - RAII 封装的读写锁
  - `ReadGuard` / `WriteGuard` 自动锁管理
  - 基于 `std::shared_mutex`
- ✅ **LockManager** (`LockManager.h/cpp`)
  - 单例模式锁管理器
  - 表级读写锁：`acquireTableLock()`
  - 索引级读写锁：`acquireIndexLock()`
  - 文件级读写锁：`acquireFileLock()`
  - 自动锁释放

#### 8.3 事务管理层 (`src/server/transaction/`)
- ✅ **Transaction** (`Transaction.h/cpp`)
  - 事务状态管理（ACTIVE, COMMITTED, ABORTED）
  - 操作记录追踪（INSERT/UPDATE/DELETE）
  - 事务 ID 生成
- ✅ **WALManager** (`WALManager.h/cpp`)
  - Write-Ahead Logging 预写日志
  - 日志序列化（`data/wal/wal.log`）
  - 崩溃恢复机制
  - fsync 保证持久化
- ✅ **TransactionManager** (`TransactionManager.h/cpp`)
  - 事务生命周期管理
  - BEGIN/COMMIT/ROLLBACK 支持
  - `thread_local` 事务隔离
  - 自动事务创建

#### 8.4 网络层 (`src/server/network/`)
- ✅ **SqlProtocol** (`SqlProtocol.h/cpp`)
  - SQL 请求解析（`SqlRequest`）
  - SQL 响应构建（`SqlResponse`）
  - 文本协议格式（类似 MySQL）
- ✅ **TcpConnection** (`TcpConnection.h/cpp`)
  - 连接缓冲区管理
  - 按行读取（`\n` 分隔）
  - 非阻塞发送
- ✅ **ThreadPool** (`ThreadPool.h/cpp`)
  - 可配置线程数
  - 任务队列（`std::function`）
  - 优雅停止
- ✅ **Reactor** (`Reactor.h/cpp`)
  - epoll 事件循环
  - 监听 socket 管理
  - 客户端连接处理
  - 读事件分发
- ✅ **TcpServer** (`TcpServer.h/cpp`)
  - SQL 执行器集成（`Executor::getInstance()`）
  - SQL 解析 → 执行 → 结果封装
  - 错误处理（`MiniSQLException` 捕获）
  - 信号处理（SIGINT/SIGTERM）

#### 8.5 存储层集成
- ✅ **FileIO 加锁**：`FileIO::writeToFile/readFromFile/appendToFile` 使用 `LockManager`
- ✅ **BPlusTree 加锁**：`BPlusTree::insert/search/remove` 使用 `LockManager`
- ✅ 表级锁保证并发安全

#### 8.6 构建系统
- ✅ **CMake 条件编译**
  - `BUILD_SERVER=ON` 启用服务器模式
  - 服务器模式不链接 Qt
  - `build_server/` 输出目录
  - `pthread` 链接
- ✅ **循环依赖解决**
  - executor ↔ optimizer 循环依赖
  - 使用 `-Wl,--start-group` / `--end-group` 链接器组
- ✅ **模块依赖修正**
  - `storage` 在服务器模式链接 `server_concurrency`
  - `executor` 添加 `SubqueryScanOperator.cpp` / `NestedLoopJoinOperator.cpp`

#### 8.7 内存管理修复
- ✅ **Result<T> 双释放修复**
  - 添加 `release()` 方法转移所有权
  - `TcpServer` 使用 `execResult.release()` 避免 `~Result()` 删除后悬空指针

#### 8.8 测试覆盖
- ✅ **test_concurrency**: 5/5 通过（RWLock, LockManager）
- ✅ **test_transaction**: 全部通过（Transaction, WALManager, TransactionManager）
- ✅ **test_network**: 2/2 通过（TcpConnection, ThreadPool, Reactor）

#### 8.9 服务器测试
**测试命令**:
```bash
# 编译服务器
cd build_server
cmake -DBUILD_SERVER=ON ..
make minisql_server -j8

# 启动服务器
./bin/minisql_server &

# 测试 SQL（使用 nc 一次性发送完整语句）
echo "CREATE TABLE users(id INT PRIMARY KEY, name VARCHAR(50));" | nc localhost 3306
echo "INSERT INTO users VALUES(1, 'Alice');" | nc localhost 3306
echo "SELECT * FROM users;" | nc localhost 3306
echo "UPDATE users SET name = 'Charlie' WHERE id = 1;" | nc localhost 3306
echo "DELETE FROM users WHERE id = 1;" | nc localhost 3306
echo "DROP TABLE users;" | nc localhost 3306
```

**测试结果**:
- ✅ CREATE TABLE 成功
- ✅ INSERT 成功
- ✅ SELECT 成功（返回正确结果）
- ✅ UPDATE 成功
- ✅ DELETE 成功
- ✅ DROP TABLE 成功
- ✅ 错误处理正常（查询不存在的表返回错误）
- ✅ 多客户端并发连接正常

**协议格式**:
```
请求: <SQL>;\n
响应成功:
  OK\n
  <message>\n
  <row_count>\n
  <col1>,<col2>,...\n
  <row1_col1>,<row1_col2>,...\n
  ...

响应错误:
  ERROR\n
  <error_message>\n
```

#### 8.10 新增文件清单
```
src/server/
├── concurrency/
│   ├── RWLock.h/cpp              # 读写锁封装
│   └── LockManager.h/cpp         # 锁管理器
├── transaction/
│   ├── Transaction.h/cpp         # 事务类
│   ├── TransactionManager.h/cpp  # 事务管理器
│   └── WALManager.h/cpp          # WAL 日志管理器
├── network/
│   ├── SqlProtocol.h/cpp         # SQL 协议
│   ├── TcpConnection.h/cpp       # TCP 连接
│   ├── ThreadPool.h/cpp          # 线程池
│   ├── Reactor.h/cpp             # Reactor 事件循环
│   └── TcpServer.h/cpp           # TCP 服务器
└── main_server.cpp               # 服务器入口

tests/server/
├── test_concurrency.cpp          # 并发控制测试
├── test_transaction.cpp          # 事务测试
└── test_network.cpp              # 网络层测试

build_server/
└── bin/
    ├── minisql_server            # 服务器可执行文件
    ├── data/wal/                 # WAL 日志目录
    └── data/                     # 数据目录
```

#### 8.11 CMake 修改
- ✅ 根 `CMakeLists.txt`: 添加 `BUILD_SERVER` 选项和链接器组
- ✅ `src/storage/CMakeLists.txt`: 服务器模式链接 `server_concurrency`
- ✅ `src/executor/CMakeLists.txt`: 添加缺失的 .cpp 文件
- ✅ `src/optimizer/CMakeLists.txt`: 修复循环依赖
- ✅ `.gitignore`: 忽略 `build_server/` 和 `build_linux/`

---

## 未实现功能

| 模块 | 功能 | 预估工作量 |
|------|------|-----------|
| **Parser** | 比较运算符子查询 `(SELECT ...) > ANY/ALL (...)` | 50-100 行 |
| **Executor** | 关联子查询处理（需外部行上下文） | 200-300 行 |
| **Executor** | 比较运算符子查询 ALL/ANY | 100-150 行 |
| **Executor** | 子查询去关联化 | 100-200 行 |
| **Executor** | 外键 CASCADE ON DELETE/UPDATE | 100-150 行 |
| **Executor** | 外键自引用支持 | 50-100 行 |
| **Executor** | DELETE 时外键引用检查 | 50-100 行 |
| **Parser+Executor** | INSERT/UPDATE 数据类型验证 | 50-100 行 |
| **Parser+Executor** | 非空约束验证 | 50-100 行 |
| **Parser+Executor** | 主键/唯一约束检查 | 50-100 行 |
| **Parser+Executor** | 默认值处理 | 50-100 行 |
| **Optimizer** | Hash Join | 200-300 行 |
| **Optimizer** | Sort-Merge Join | 200-300 行 |
| **Optimizer** | 多表连接顺序优化 | 100-200 行 |
| **Optimizer** | 利用索引消除排序 | 100-150 行 |
| **Optimizer** | 冗余排序消除 | 50-100 行 |
| **Optimizer** | JOIN 条件下推 | 100-150 行 |
| **Optimizer** | WHERE 条件下推到子查询 | 50-100 行 |
| **Optimizer** | 聚合后条件下推 | 50-100 行 |
| **Storage** | B+ 树节点合并与借键 | 150-200 行 |
| **Storage** | 事务支持（ACID + MVCC + WAL） | 500-800 行 |
| **Storage** | 视图（CREATE VIEW + 查询重写） | 200-300 行 |
| **Storage** | 存储过程（CALL 执行） | 300-400 行 |
| **Storage** | 触发器（BEFORE/AFTER 事件） | 300-400 行 |

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
./bin/test_subquery.exe
./bin/test_integration.exe
./bin/test_foreign_key.exe
./bin/test_join.exe
./bin/test_from_subquery.exe
./bin/test_having.exe

# 优化器测试
./bin/test_rule_optimizer.exe
./bin/test_cost_optimizer.exe
./bin/test_statistics.exe
./bin/test_query_rewriter.exe
./bin/test_index_selector.exe
./bin/test_plan_generator.exe
./bin/test_explain.exe
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
- **test_rule_optimizer**: 通过
- **test_cost_optimizer**: 通过
- **test_statistics**: 通过
- **test_query_rewriter**: 通过
- **test_index_selector**: 通过
- **test_plan_generator**: 通过
- **test_explain**: 通过
- **test_integration**: 通过
- **test_subquery**: 通过
- **test_foreign_key**: 通过
- **test_join**: 通过 (Phase 7)
- **test_from_subquery**: 通过 (Phase 7)
- **test_having**: 通过 (Phase 7)

**累计测试**: 29/29 测试套件全部通过
- **test_join**: 通过 (Phase 7)
- **test_from_subquery**: 通过 (Phase 7)
- **test_having**: 通过 (Phase 7)

**累计测试**: 1400+ 测试通过 (29 个测试套件全部通过)

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
│   ├── executor/        # 查询执行器 (Phase 4, Phase 7)
│   │   ├── ExecutionOperator.h/cpp
│   │   ├── ExpressionEvaluator.h/cpp
│   │   ├── TableScanOperator.h/cpp
│   │   ├── FilterOperator.h/cpp
│   │   ├── ProjectOperator.h/cpp
│   │   ├── NestedLoopJoinOperator.h/cpp
│   │   ├── AggregateOperator.h/cpp
│   │   ├── SortOperator.h/cpp
│   │   ├── LimitOperator.h/cpp
│   │   ├── SubqueryScanOperator.h/cpp  # Phase 7: FROM 子查询
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
│   ├── test_executor.cpp
│   ├── test_subquery.cpp
│   ├── test_integration.cpp
│   ├── test_foreign_key.cpp
│   ├── test_join.cpp              # Phase 7
│   ├── test_from_subquery.cpp     # Phase 7
│   └── test_having.cpp           # Phase 7
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

详见上方 **未实现功能** 表格，按模块分类列出完整待办清单。

---

## 下一阶段建议

1. **数据类型与约束验证**（100-200 行）— 补全基础验证
2. **子查询执行增强**（200-300 行）— 支持标量/关联子查询
3. **谓词下推**（150-200 行）— 优化器增强
4. **连接优化**（300-400 行）— Hash Join / Sort-Merge Join
5. **外键约束增强**（150-200 行）— CASCADE / 自引用
6. **事务管理**（500-800 行）— ACID + MVCC + WAL

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
- **964b932**: feat(executor): 实现查询执行器（Volcano 模型、全部执行算子、DML/DDL 执行器）
- **964b932**: feat(executor): 实现查询执行器（Volcano 模型、全部执行算子、DML/DDL 执行器）
- **(Phase 7)**: feat(Phase 7): JOIN 执行 + FROM 子查询 + HAVING + 索引选择恢复 + Result<T> 悬空指针修复
- **(Phase 8)**: feat(Phase 8): Linux 服务器实现 - Reactor + Thread Pool + 事务管理 + WAL + 并发控制 + SQL 执行器集成

---

## 联系方式

- **GitHub**: https://github.com/chenzhou0071/SQL_Management_System
- **分支**: main

---

