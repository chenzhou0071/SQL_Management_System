# MiniSQL 开发手册

## 项目概述

**项目名称**: MiniSQL - 轻量级 MySQL 风格数据库管理系统
**技术栈**: C++17, Qt 6.5.3, CMake, MinGW
**开发模式**: 模块化、分阶段开发

---

## 开发进度

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
- ✅ 语句节点：`SelectStmt`, `InsertStmt`, `UpdateStmt`, `DeleteStmt`, `CreateTableStmt`, `DropStmt`, `UseStmt`, `ShowStmt`
- ✅ 表引用节点：`TableRef`, `JoinClause`
- ✅ 列定义节点：`ColumnDefNode`, `ConstraintDefNode`

#### 3.3 语法分析器 (`Parser.h/cpp`)
- ✅ USE 语句：`USE database;`
- ✅ SHOW 语句：`SHOW DATABASES;`, `SHOW TABLES;`
- ✅ SELECT 语句：
  - SELECT 列表、DISTINCT
  - FROM 子句
  - WHERE 子句
  - JOIN 子句（INNER, LEFT, RIGHT, FULL, CROSS）
  - GROUP BY、HAVING
  - ORDER BY、LIMIT、OFFSET
- ✅ INSERT 语句：`INSERT INTO table (cols) VALUES (...);`
- ✅ UPDATE 语句：`UPDATE table SET col=val WHERE ...;`
- ✅ DELETE 语句：`DELETE FROM table WHERE ...;`
- ✅ CREATE TABLE 语句（支持 IF NOT EXISTS、列约束）
- ✅ DROP TABLE 语句（支持 IF EXISTS）
- ✅ 表达式解析（递归下降 + Pratt Parser 技术处理优先级）

#### 3.4 语义分析器 (`SemanticAnalyzer.h/cpp`)
- ✅ 表存在性检查
- ✅ 字段存在性检查
- ✅ 数据类型验证
- ✅ 约束检查（重复列名、空列名、表已存在等）
- ✅ 表达式验证（列引用检查）

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

#### 2.3 B+树索引 (`BPlusTree.h/cpp`)
- ✅ 基本操作：插入、查找、删除
- ✅ 范围查询：支持 [min, max] 范围搜索
- ✅ 树分裂：自动分裂和树高度调整
- ✅ 叶子节点链表：支持顺序遍历
- ✅ 可配置阶数

**B+树特点**:
- 所有数据在叶子节点
- 叶子节点双向链表连接
- 内部节点只存储索引键
- 支持重复键检测

#### 2.4 索引管理 (`IndexManager.h/cpp`)
- ✅ 索引创建：主键、唯一、普通索引
- ✅ 索引删除
- ✅ 索引查找
- ✅ 运行时索引缓存

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

## Phase 2 未实现功能

### ⚠️ 索引持久化 (简化实现)

**问题描述**:
- `saveIndex()` 只写入 `INDEX:indexName` 字符串
- `loadIndex()` 只检查文件存在，未加载 B+ 树数据

**影响**:
- ❌ 程序重启后，索引数据丢失
- ❌ 下次启动时索引为空，需要重建

**解决方案** (未实现):
需要完整的 B+ 树序列化：
1. 前序/层序遍历树结构
2. 序列化节点数据（keys、values、children）
3. 重建时恢复树的所有连接关系

**预估工作量**: 200-300 行代码

---

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
**当前实现**: 仅存储标记 `INDEX:indexName`
**完整实现**: 需要序列化 B+ 树结构（未实现）

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
cmake --build . --target storage
```

### 运行测试
```bash
# 公共模块测试
./bin/test_common.exe

# 存储模块测试
./bin/test_storage.exe

# 文件生成演示
./bin/demo_file_creation.exe
```

### 测试结果
- **test_common**: 60/60 通过
- **test_storage**: 1134/1134 通过
- **test_lexer**: 122/122 通过
- **test_ast**: 10/10 通过
- **test_parser**: 40/40 通过
- **test_semantic**: 16/16 通过

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
│   └── demo_file_creation.cpp
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

1. **索引持久化**: 重启后索引数据丢失，需要重建
2. **外键约束**: 定义已支持，但未实现约束检查
3. **事务支持**: 未实现
4. **并发控制**: 未实现
5. **视图**: 未实现
6. **存储过程**: 未实现
7. **触发器**: 未实现

---

## 下一阶段规划

### Phase 3: SQL 解析器 (已完成)
- ✅ 词法分析器 (Lexer)
- ✅ 语法分析器 (Parser)
- ✅ 抽象语法树 (AST)
- ✅ 语义分析器 (SemanticAnalyzer)

### Phase 4: 查询执行器 (未开始)
- 查询计划生成
- 执行引擎
- 结果集处理

### Phase 5: 事务管理 (未开始)
- ACID 特性
- 并发控制
- 日志与恢复

---

## 技术债务

### 高优先级
- [ ] 完善索引持久化和加载
- [ ] 外键约束检查
- [ ] 数据类型验证（插入时）

### 中优先级
- [ ] B+树删除算法完善（目前是简化版）
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

---

## Phase 3 未实现功能

### ⚠️ ALTER TABLE 语句

**当前状态**: 未实现
**需要功能**:
- 添加/删除列
- 修改列类型
- 添加/删除约束
- 重命名表

**预估工作量**: 100-150 行代码

---

### ⚠️ 函数调用表达式

**当前状态**: 部分实现（仅占位符）
**需要功能**:
- 聚合函数：`COUNT`, `SUM`, `AVG`, `MIN`, `MAX`
- 字符串函数：`CONCAT`, `SUBSTRING`, `UPPER`, `LOWER`
- 日期函数：`NOW`, `DATE`, `TIME`
- 类型转换：`CAST`, `CONVERT`

**预估工作量**: 150-200 行代码

---

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

## 联系方式

- **GitHub**: https://github.com/chenzhou0071/SQL_Management_System
- **分支**: main

---

*最后更新: 2026-03-26*
