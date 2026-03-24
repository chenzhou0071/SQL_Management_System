# MiniSQL 数据库管理系统设计文档

## 一、项目概述

### 1.1 项目背景

MiniSQL 是一款基于 C++17 与 Qt5/6 开发的轻量级关系型数据库管理系统（DBMS），核心目标是实现 MySQL 核心语法的解析与执行，支持完整的 DDL、DML、DQL 操作，同时提供图形化界面（GUI）与命令行终端（CLI）双模式操作。

本项目定位为**简历项目**，重点展示：
- 底层原理的理解与实现能力
- 数据库系统的核心技术深度
- 工程化开发能力

### 1.2 技术栈

| 类别 | 选型 |
|------|------|
| 编程语言 | C++17 |
| GUI框架 | Qt 5.15/6.x |
| 构建工具 | CMake |
| 目标平台 | Windows |
| 数据存储 | 文本文件持久化 |

### 1.3 核心功能模块

| 模块 | 功能 |
|------|------|
| SQL解析层 | 词法分析、语法分析、语义检查、执行计划生成 |
| 优化器 | 基于规则的优化（RBO）、索引选择 |
| 执行器 | 表达式计算、数据扫描、JOIN执行 |
| 存储引擎 | 文本文件持久化、B+树索引 |
| 事务管理 | 基础事务支持（原子性、一致性、持久性） |
| 数据类型 | 基础类型（INT/VARCHAR/DATE等）、约束（PRIMARY KEY/UNIQUE/NOT NULL/FOREIGN KEY） |
| 进阶功能 | 视图、存储过程、触发器 |
| 双模式 | GUI图形界面 + CLI终端模式 |

---

## 二、系统架构

```
┌─────────────────────────────────────────────────────────────┐
│                     应用层（Application）                    │
│   ┌─────────────────┐         ┌─────────────────┐          │
│   │   GUI界面(Qt)    │  <--->  │  CLI终端模式    │          │
│   │  可视化操作/表格  │         │  类mysql命令行   │          │
│   └────────┬────────┘         └────────┬────────┘          │
│            │                            │                    │
│            └────────────┬───────────────┘                    │
│                         ▼                                    │
│            ┌────────────────────────┐                        │
│            │    SQL执行入口接口     │                        │
│            │   executeSQL(string)   │                        │
│            └────────────┬───────────┘                        │
└─────────────────────────┼────────────────────────────────────┘
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                    解析层（Parser）                          │
│   ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│   │   词法分析   │  │   语法分析   │  │   语义分析   │      │
│   │   (Lexer)   │─▶│   (Parser)   │─▶│  (Analyzer)  │      │
│   └──────────────┘  └──────────────┘  └──────────────┘      │
│                            │                                 │
│                            ▼                                 │
│                   ┌──────────────────┐                       │
│                   │  生成执行计划    │                       │
│                   │  (ExecutionPlan) │                       │
│                   └────────┬─────────┘                       │
└─────────────────────────────┼─────────────────────────────────┘
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    优化器（Optimizer）                        │
│   ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│   │  规则优化器   │  │  索引选择器   │  │  执行计划    │      │
│   │    (RBO)    │  │              │  │   生成器     │      │
│   └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────┼─────────────────────────────────┘
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    执行器（Executor）                        │
│   ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│   │  表达式计算  │  │   数据扫描   │  │   JOIN执行   │      │
│   │              │  │              │  │ (NestedLoop) │      │
│   └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────┼─────────────────────────────────┘
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    存储引擎（Storage）                        │
│   ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│   │   表管理     │  │   索引管理   │  │   文件I/O    │      │
│   │  (TableMgr) │  │  (B+Tree)   │  │ (TextFile)   │      │
│   └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
```

---

## 三、SQL解析层设计

### 3.1 词法分析器（Lexer）

**输入**：SQL字符串
**输出**：Token流

**Token类型**：

| 类型 | 说明 | 示例 |
|------|------|------|
| KEYWORD | 关键字 | SELECT, INSERT, CREATE |
| IDENTIFIER | 标识符 | 表名, 列名 |
| LITERAL | 字面量 | 字符串, 数字 |
| OPERATOR | 操作符 | =, >, <, +, - |
| DELIMITER | 分隔符 | , ; ( ) |
| COMMENT | 注释 | --, /* */ |
| DOT | 点号 | . |

**实现要点**：
- 状态机模式实现
- 识别单引号/双引号字符串
- 识别数字常量
- 识别注释并跳过
- 关键字大小写不敏感

### 3.2 语法分析器（Parser）

**采用递归下降（Recursive Descent）算法**

**语法规则（BNF）**：

```
statement    → select_stmt | insert_stmt | update_stmt
              | delete_stmt | create_stmt | alter_stmt
              | drop_stmt | show_stmt | use_stmt

select_stmt  → SELECT select_expr_list FROM table_ref
              [WHERE where_condition]
              [GROUP BY column_list]
              [HAVING where_condition]
              [ORDER BY order_list]
              [LIMIT limit_count]

insert_stmt  → INSERT INTO table_name
              [column_list] VALUES value_list

update_stmt  → UPDATE table_name
              SET column_list WHERE where_condition

delete_stmt  → DELETE FROM table_name
              [WHERE where_condition]

create_stmt  → CREATE TABLE table_name
              (column_def_list) [table_options]
```

### 3.3 支持的SQL命令

| 类别 | 命令 | 说明 |
|------|------|------|
| DDL | CREATE DATABASE/TABLE/INDEX/VIEW/PROCEDURE/TRIGGER | 库/表/索引/视图/存储过程/触发器 |
| DML | INSERT, UPDATE, DELETE | 数据增删改 |
| DQL | SELECT（含完整子查询支持） | 查询 |
| DCL | SHOW DATABASES/TABLES/INDEX | 显示信息 |
| 其他 | USE, DROP, TRUNCATE, ALTER | 切换/删除/清空/修改 |

---

## 四、优化器与执行器设计

### 4.1 基础优化器（RBO）

**优化规则（按优先级）**：

**优先级1：索引规则**
- 主键/唯一索引优先
- 复合索引最左前缀匹配
- 避免全表扫描条件

**优先级2：连接顺序优化**
- 小表驱动大表（Nested Loop）
- 谓词下推

**优先级3：SQL改写**
- 常量折叠
- 消除冗余排序

### 4.2 索引选择算法

```
1. 收集可用索引
2. 评估每个索引的 selectivity
3. 选择 selectivity 最高的索引
4. 生成扫描计划（Index Scan / Table Scan）
```

### 4.3 执行器架构

| 组件 | 功能 |
|------|------|
| Project | 投影运算（列筛选） |
| Sort | 排序运算（ORDER BY） |
| Filter | 过滤运算（WHERE/HAVING） |
| Join | 连接运算（Nested Loop） |
| Aggregate | 聚合运算（GROUP BY） |
| Table Scan | 表扫描 |

### 4.4 EXPLAIN 执行计划

```
EXPLAIN SELECT * FROM users WHERE age > 18

输出示例：
+----+-------------+-------+------+---------------+
| id | select_type| table | type | key           |
+----+-------------+-------+------+---------------+
|  1 | SIMPLE     | users | range| idx_age       |
+----+-------------+-------+------+---------------+
```

---

## 五、存储与索引设计

### 5.1 文件存储结构

```
项目根目录/
├── data/                              # 数据存储目录
│   ├── database1/                     # 数据库目录
│   │   ├── table1.meta               # 表结构定义（JSON）
│   │   ├── table1.data               # 表数据文件（CSV）
│   │   ├── table1.idx                # 索引文件
│   │   ├── view1.view                # 视图定义
│   │   ├── proc1.proc                # 存储过程定义
│   │   └── trigger1.trigger           # 触发器定义
│   └── database2/
└── log/                               # 日志目录
```

### 5.2 表结构文件 (.meta)

```json
{
    "table_name": "users",
    "engine": "MiniSQL",
    "columns": [
        {"name": "id", "type": "INT", "primary_key": true},
        {"name": "name", "type": "VARCHAR", "length": 50, "not_null": true},
        {"name": "age", "type": "INT", "default": 0},
        {"name": "email", "type": "VARCHAR", "length": 100, "unique": true}
    ],
    "indexes": [
        {"name": "PRIMARY", "type": "BTREE", "columns": ["id"]},
        {"name": "idx_email", "type": "BTREE", "columns": ["email"]}
    ],
    "foreign_keys": [
        {"name": "fk_dept", "column": "dept_id", "ref_table": "departments", "ref_column": "id"}
    ]
}
```

### 5.3 数据文件 (.data)

```
格式：CSV风格
示例：
1,Alice,20,alice@example.com
2,Bob,25,bob@example.com
3,Charlie,30,charlie@example.com

特殊处理：
- 字符串用单引号包裹：'Hello World'
- NULL用\N表示：\N
```

### 5.4 B+树索引

```
特点：
- 所有数据在叶子节点
- 叶子节点链表连接
- 支持范围查询
- 树高控制在3-4层
```

### 5.5 索引类型

| 索引类型 | 创建方式 | 使用场景 |
|----------|----------|----------|
| PRIMARY KEY | CREATE TABLE时自动创建 | 主键查找、唯一快速定位 |
| UNIQUE | CREATE TABLE 或 CREATE INDEX | 唯一约束、快速查找 |
| INDEX | CREATE INDEX | 普通加速 |
| 复合索引 | CREATE INDEX (col1,col2) | 多列查询、最左前缀匹配 |

---

## 六、数据类型与约束设计

### 6.1 支持的数据类型

**数值类型**：

| 类型 | 存储大小 | 范围 |
|------|----------|------|
| TINYINT | 1字节 | -128 ~ 127 |
| SMALLINT | 2字节 | -32768 ~ 32767 |
| INT | 4字节 | -2^31 ~ 2^31-1 |
| BIGINT | 8字节 | -2^63 ~ 2^63-1 |
| FLOAT | 4字节 | 单精度浮点 |
| DOUBLE | 8字节 | 双精度浮点 |
| DECIMAL(p,s) | 变长 | 精确数值 |

**字符串类型**：

| 类型 | 最大长度 |
|------|----------|
| CHAR(n) | 255字符 |
| VARCHAR(n) | 65535字节 |
| TEXT | 64KB |
| TINYTEXT | 255字节 |
| LONGTEXT | 4GB |

**日期时间类型**：

| 类型 | 格式 |
|------|------|
| DATE | YYYY-MM-DD |
| TIME | HH:MM:SS |
| DATETIME | YYYY-MM-DD HH:MM:SS |
| TIMESTAMP | Unix时间戳 |

**其他类型**：
- BOOLEAN → TINYINT(1) 别名
- JSON → JSON格式字符串

### 6.2 约束定义

| 约束 | 关键字 | 说明 |
|------|--------|------|
| 主键 | PRIMARY KEY | 唯一 + NOT NULL，自动建索引 |
| 外键 | FOREIGN KEY | 引用完整性，级联操作 |
| 唯一 | UNIQUE | 唯一值，允许NULL |
| 非空 | NOT NULL | 不允许空值 |
| 默认值 | DEFAULT value | 插入时默认值 |
| 自增 | AUTO_INCREMENT | 自动递增序号 |

---

## 七、GUI界面设计

### 7.1 主界面布局

```
┌─────────────────────────────────────────────────────────────────┐
│  MiniSQL  [当前数据库: test_db]              [主题] [刷新]       │
├─────────────────────────────────────────────────────────────────┤
│  [新建库] [新建表] [插入] [修改] [删除] [查询] [EXPLAIN]        │
├─────────────────┬───────────────────────────────────────────────┤
│                 │                                               │
│  ▼ 数据库       │   数据展示区                                  │
│    ▶ test_db   │   ┌────┬──────┬──────┐                        │
│        ├ users │   │ id │ name │ age  │                        │
│      * orders  │   ├────┼──────┼──────┤                        │
│        └ dept  │   │ 1  │ Tom  │ 20   │                        │
│                 │   │ 2  │ Jane │ 25   │                        │
│                 │   │ 3  │ Bob  │ 30   │                        │
│                 │   └────┴──────┴──────┘                        │
│                 │   (点击单元格可直接编辑)                        │
│                 │                                               │
├─────────────────┴───────────────────────────────────────────────┤
│  终端区（输入输出合并）                                          │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │ mysql> SELECT * FROM users;                                ││
│  │ ┌────┬──────┬──────┐                                      ││
│  │ │ id │ name │ age  │                                      ││
│  │ ├────┼──────┼──────┤                                      ││
│  │ │ 1  │ Tom  │ 20   │                                      ││
│  │ │ 2  │ Jane │ 25   │                                      ││
│  │ └────┴──────┴──────┘                                      ││
│  │ 2 rows in set (0.00s)                                      ││
│  │                                                             ││
│  │ mysql> _                                                   ││
│  └─────────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────────┘
```

### 7.2 交互方式

1. **手动输入**：在终端区输入SQL命令，按Enter执行
2. **按钮操作**：点击按钮 → 弹出对话框填写参数 → 自动生成SQL到终端执行
3. **数据编辑**：点击表格单元格直接修改 → 自动生成UPDATE语句执行
4. **自动保存**：每次DML操作后自动持久化到文件

### 7.3 工具栏功能

所有按钮均为弹窗模式，点击按钮弹出对话框填写参数，确认后自动生成SQL到终端执行。

| 按钮 | 功能 | 弹窗内容 |
|------|------|----------|
| 新建库 | 创建数据库 | 输入库名 |
| 新建表 | 创建表 | 表名、字段列表（名称、类型、约束） |
| 插入 | 插入数据 | 各字段输入框（显示当前表结构） |
| 修改 | 更新数据 | SET字段=新值 WHERE 条件 |
| 删除 | 删除数据 | WHERE 条件 |
| 查询 | SELECT查询 | 选择字段、WHERE条件、ORDER BY、LIMIT等 |
| EXPLAIN | 执行计划 | 选择查询语句，显示执行计划 |
| 刷新 | 重新加载当前表数据 | 自动执行SELECT * FROM 当前表 |
| 主题 | 切换亮色/暗色主题 | 自动切换 |

### 7.4 弹窗示例

**查询弹窗**：
```
┌─────────────────────────────────────────────────────┐
│              查询 - users                            │
├─────────────────────────────────────────────────────┤
│  字段选择：                                          │
│  ☑ id   ☑ name   ☑ age   ☑ email                 │
│                                                     │
│  WHERE条件：                                         │
│  字段: [age    ▼]  操作符: [>  ▼]  值: [18      ]     │
│                                                     │
│  ORDER BY:  [id ▼]  [ASC ▼]                        │
│  LIMIT:     [10      ]                              │
│                                                     │
│            [取消]              [执行查询]             │
└─────────────────────────────────────────────────────┘
```

**EXPLAIN弹窗**：
```
┌─────────────────────────────────────────────────────┐
│              EXPLAIN                                 │
├─────────────────────────────────────────────────────┤
│  ┌───────────────────────────────────────────────┐  │
│  │ SELECT * FROM users WHERE                     │  │
│  │ id = 1                                        │  │
│  │ ORDER BY age DESC                             │  │
│  │ LIMIT 10                                      │  │
│  └───────────────────────────────────────────────┘  │
│  (按Enter换行，Ctrl+Enter执行)                       │
│            [取消]              [执行EXPLAIN]         │
└─────────────────────────────────────────────────────┘
```

---

## 八、进阶功能设计

### 8.1 视图（View）

```
创建：
CREATE VIEW view_name AS select_stmt

示例：
CREATE VIEW user_stats AS
SELECT department, COUNT(*) as count, AVG(age) as avg_age
FROM users GROUP BY department;

特点：
- 不存储实际数据，存储查询定义
- 查询时执行定义中的SELECT
```

### 8.2 存储过程（Stored Procedure）

```
创建：
CREATE PROCEDURE procedure_name(IN param1 INT, OUT param2 VARCHAR(50))
BEGIN
    DECLARE variable1 INT DEFAULT 0;
    SELECT * FROM users WHERE id = param1;
    SET param2 = 'completed';
END;

调用：
CALL procedure_name(1, @result);

支持：
- IN/OUT/INOUT参数
- 局部变量DECLARE
- 流程控制IF/WHILE
- 事务控制COMMIT/ROLLBACK
```

### 8.3 触发器（Trigger）

```
创建：
CREATE TRIGGER trigger_name
{BEFORE | AFTER} {INSERT | UPDATE | DELETE}
ON table_name
FOR EACH ROW
BEGIN
    -- NEW表示新行，OLD表示旧行
END;

示例：
CREATE TRIGGER user_delete_log
AFTER DELETE ON users
FOR EACH ROW
BEGIN
    INSERT INTO delete_log (deleted_id)
    VALUES (OLD.id);
END;
```

### 8.4 文件格式

**视图 (.view)**：
```json
{
    "name": "user_stats",
    "definition": "SELECT department, COUNT(*) FROM users GROUP BY department"
}
```

**存储过程 (.proc)**：
```json
{
    "name": "get_user_by_id",
    "parameters": [
        {"name": "p_id", "type": "IN", "data_type": "INT"},
        {"name": "p_name", "type": "OUT", "data_type": "VARCHAR", "length": 50}
    ],
    "body": "SELECT name INTO p_name FROM users WHERE id = p_id;"
}
```

**触发器 (.trigger)**：
```json
{
    "name": "user_delete_log",
    "table": "users",
    "timing": "AFTER",
    "event": "DELETE",
    "body": "INSERT INTO delete_log (deleted_id) VALUES (OLD.id);"
}
```

---

## 九、事务管理设计

### 9.1 事务概述

**事务**：一组 SQL 操作，要么全部成功，要么全部失败。

**ACID 特性**：

| 特性 | 说明 | MiniSQL 实现 |
|------|------|--------------|
| **原子性** (Atomicity) | 要么全做，要么全不做 | ✓ 操作日志 + 回滚 |
| **一致性** (Consistency) | 数据始终保持有效状态 | ✓ 约束检查 |
| **隔离性** (Isolation) | 多个事务互不干扰 | ✗ 单线程，无并发 |
| **持久性** (Durability) | 提交后永久保存 | ✓ 文件写入 |

### 9.2 事务语法

```sql
-- 开始事务
START TRANSACTION;
-- 或
BEGIN;

-- 执行 SQL
UPDATE accounts SET balance = balance - 100 WHERE id = 1;
UPDATE accounts SET balance = balance + 100 WHERE id = 2;

-- 提交（确认）
COMMIT;

-- 或回滚（取消）
ROLLBACK;
```

### 9.3 事务实现机制

```
事务状态流转：

┌─────────┐
│  开始   │ START TRANSACTION
└────┬────┘
     │
     ▼
┌─────────┐
│ 进行中  │ ← 记录操作日志，修改内存
└────┬────┘
     │
     ├─────────────┐
     │             │
     ▼             ▼
┌─────────┐   ┌─────────┐
│ COMMIT  │   │ROLLBACK │
└────┬────┘   └────┬────┘
     │             │
     ▼             ▼
┌─────────┐   ┌─────────┐
│ 写文件   │   │ 撤销操作 │
│ 清日志   │   │ 清日志   │
└─────────┘   └─────────┘
```

### 9.4 操作日志设计

```
操作日志结构：

class Operation {
    enum Type { INSERT, UPDATE, DELETE };
    Type type;
    string tableName;
    Row oldRow;      // UPDATE/DELETE 时记录旧行
    Row newRow;      // INSERT/UPDATE 时记录新行

    void execute() { /* 执行操作 */ }
    void undo() { /* 撤销操作 */ }
};

示例：
UPDATE users SET age = 25 WHERE id = 1;

操作日志：
{
    type: UPDATE,
    table: "users",
    oldRow: {id: 1, name: "Tom", age: 20},
    newRow: {id: 1, name: "Tom", age: 25}
}

回滚时：执行 UPDATE users SET age = 20 WHERE id = 1;
```

### 9.5 事务管理器

```cpp
class TransactionManager {
private:
    bool inTransaction = false;
    vector<Operation> log;

public:
    void begin() {
        inTransaction = true;
        log.clear();
    }

    void addOperation(Operation op) {
        if (inTransaction) {
            log.push_back(op);
        } else {
            op.execute();  // 自动提交模式
        }
    }

    void commit() {
        // 执行所有操作
        for (auto& op : log) {
            op.execute();
        }
        // 写入文件
        flushToFile();
        inTransaction = false;
        log.clear();
    }

    void rollback() {
        // 撤销操作（倒序）
        for (int i = log.size()-1; i >= 0; i--) {
            log[i].undo();
        }
        inTransaction = false;
        log.clear();
    }

    bool isInTransaction() const {
        return inTransaction;
    }
};
```

### 9.6 使用示例

```sql
-- 银行转账
START TRANSACTION;

UPDATE accounts SET balance = balance - 100 WHERE user_id = 1;
UPDATE accounts SET balance = balance + 100 WHERE user_id = 2;

-- 如果成功
COMMIT;

-- 如果失败
ROLLBACK;
```

### 9.7 事务与存储过程结合

```sql
CREATE PROCEDURE transfer_money(
    IN from_user INT,
    IN to_user INT,
    IN amount DECIMAL(10,2)
)
BEGIN
    DECLARE EXIT HANDLER FOR SQLEXCEPTION
    BEGIN
        ROLLBACK;
        SELECT 'Transfer failed' AS message;
    END;

    START TRANSACTION;
    UPDATE accounts SET balance = balance - amount WHERE user_id = from_user;
    UPDATE accounts SET balance = balance + amount WHERE user_id = to_user;
    COMMIT;
    SELECT 'Transfer success' AS message;
END;
```

---

## 十、项目目录结构

```
MiniSQL/
├── src/
│   ├── main.cpp                    # 入口文件
│   ├── application/                # 应用层
│   │   ├── MainWindow.cpp          # GUI主窗口
│   │   ├── CLIMode.cpp             # 终端模式
│   │   └── ...
│   ├── parser/                     # 解析层
│   │   ├── Lexer.cpp               # 词法分析器
│   │   ├── Parser.cpp              # 语法分析器
│   │   ├── AST.cpp                 # 抽象语法树
│   │   └── ...
│   ├── optimizer/                  # 优化器
│   │   ├── Optimizer.cpp           # 优化器主类
│   │   ├── RuleOptimizer.cpp       # 规则优化
│   │   └── ...
│   ├── executor/                   # 执行器
│   │   ├── Executor.cpp            # 执行器主类
│   │   ├── Expression.cpp          # 表达式计算
│   │   ├── JoinExecutor.cpp        # JOIN执行
│   │   └── ...
│   ├── storage/                    # 存储引擎
│   │   ├── TableManager.cpp        # 表管理
│   │   ├── IndexManager.cpp        # 索引管理（B+树）
│   │   ├── FileIO.cpp              # 文件读写
│   │   └── ...
│   ├── transaction/                # 事务管理
│   │   ├── TransactionManager.cpp  # 事务管理器
│   │   ├── Operation.cpp           # 操作日志
│   │   └── ...
│   ├── common/                     # 公共组件
│   │   ├── Type.cpp                # 数据类型
│   │   ├── Value.cpp               # 值对象
│   │   └── ...
│   └── CMakeLists.txt
├── data/                           # 数据目录（运行时生成）
├── docs/
│   └── plans/                      # 设计文档
└── README.md
```

---

## 十一、技术亮点总结

1. **完整的SQL解析**：词法分析 + 语法分析 + 语义检查
2. **基础查询优化**：基于规则的优化（RBO）+ 索引选择
3. **B+树索引**：主键索引、唯一索引、复合索引
4. **基础事务支持**：原子性、一致性、持久性（操作日志 + 回滚机制）
5. **进阶SQL特性**：视图、存储过程、触发器
6. **双模式操作**：GUI图形界面 + CLI终端模式
7. **自动持久化**：每次操作自动保存，无需手动保存
8. **可视化数据编辑**：直接在表格上修改数据
