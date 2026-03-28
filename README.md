# MiniSQL - 轻量级关系型数据库管理系统

## 项目简介

MiniSQL 是一款基于 C++17 与 Qt 6.5 开发的轻量级关系型数据库管理系统（DBMS），从零实现 MySQL 核心语法，重点展示数据库底层原理和工程化实现能力。

**开发进度**：Phase 7 完成 — 高级 SELECT 支持

## 技术栈

- **编程语言**：C++17
- **GUI框架**：Qt 6.5.3
- **构建工具**：CMake + MinGW
- **目标平台**：Windows

## 核心功能

- ✅ **完整的 SQL 解析**：词法分析（Lexer）、语法分析（Parser）、语义检查（SemanticAnalyzer）
- ✅ **查询优化器**：规则优化（RBO）、代价优化（CBO）、索引选择
- ✅ **B+树索引**：主键索引、唯一索引、复合索引、持久化
- ✅ **Volcano 执行模型**：TableScan、Filter、Project、Join、Aggregate、Sort、Limit 等算子
- ✅ **高级 SELECT**：INNER/LEFT/RIGHT JOIN、FROM 子查询、GROUP BY + HAVING
- ✅ **外键约束**：INSERT/UPDATE 时级联引用检查
- ✅ **数据持久化**：JSON 元数据 + CSV 数据 + JSON 索引文件
- ⏳ **进阶特性**：视图、存储过程、触发器（规划中）

## 已完成阶段

| Phase | 内容 | 状态 |
|-------|------|------|
| Phase 1 | 基础框架（数据类型、值系统、错误处理、日志） | ✅ |
| Phase 2 | 存储引擎（B+树索引、表管理、文件操作） | ✅ |
| Phase 3 | SQL 解析器（词法/语法/语义分析、函数调用、IN/EXISTS 子查询） | ✅ |
| Phase 4 | 查询执行器（Volcano 模型、全部执行算子、DML/DDL） | ✅ |
| Phase 5 | 查询优化器（统计信息、索引选择、查询重写、代价模型、执行计划） | ✅ |
| Phase 6 | 基础功能集成（优化器入口、外键约束、IN/NOT IN/EXISTS 子查询执行） | ✅ |
| Phase 7 | 高级 SELECT（JOIN 执行、FROM 子查询、HAVING 完善、索引选择恢复） | ✅ |

## 项目结构

```
MiniSQL/
├── src/
│   ├── common/        # 公共组件（数据类型、错误处理、日志、Result<T>）
│   ├── storage/       # 存储引擎（B+树、表管理、文件操作）
│   ├── parser/        # SQL 解析层（Lexer、Parser、AST、SemanticAnalyzer）
│   ├── optimizer/     # 查询优化器（统计信息、索引选择、计划生成）
│   └── executor/      # SQL 执行器（算子体系、表达式求值、DML/DDL）
├── tests/             # 测试代码（29 个测试套件）
├── docs/plans/       # 设计文档和实现计划
└── CMakeLists.txt     # CMake 构建配置
```

## 快速开始

### 环境要求

- Qt 6.5.3
- CMake 3.15+
- MinGW / MSVC 编译器（C++17）

### 构建项目

```bash
git clone https://github.com/chenzhou0071/SQL_Management_System.git
cd SQL_Management_System
mkdir build && cd build
cmake ..
cmake --build .
```

### 运行测试

```bash
./bin/test_executor.exe        # 执行器测试
./bin/test_having.exe         # HAVING 子句测试
./bin/test_join.exe           # JOIN 执行测试
./bin/test_from_subquery.exe  # FROM 子查询测试
./bin/test_foreign_key.exe    # 外键约束测试
```

## 使用示例

```sql
-- 创建数据库和表
CREATE DATABASE test_db;
USE test_db;
CREATE TABLE users (id INT, name VARCHAR(20), dept_id INT);
CREATE TABLE orders (id INT, user_id INT, amount INT);

-- 创建索引
CREATE INDEX idx_dept ON users(dept_id);

-- 插入数据
INSERT INTO users VALUES (1, 'Alice', 1);
INSERT INTO users VALUES (2, 'Bob', 2);
INSERT INTO orders VALUES (1, 1, 100);
INSERT INTO orders VALUES (2, 1, 200);

-- 基础查询
SELECT * FROM users WHERE dept_id = 1;

-- JOIN 查询
SELECT u.name, o.amount
FROM users u
JOIN orders o ON u.id = o.user_id;

-- FROM 子查询
SELECT *
FROM (SELECT id, name FROM users WHERE dept_id = 1) AS t;

-- GROUP BY + HAVING
SELECT dept_id, COUNT(*) AS cnt
FROM users
GROUP BY dept_id
HAVING COUNT(*) > 1;

-- 查看执行计划
EXPLAIN SELECT * FROM users WHERE age > 18;
```

## 技术亮点

1. **SQL 解析器**：三阶段分析（词法/语法/语义），递归下降 + Pratt Parser
2. **Volcano 迭代器模型**：统一接口的算子树执行框架
3. **B+树索引**：手动实现，支持范围查询和持久化
4. **查询优化**：规则优化 + 代价优化，自动选择最优索引
5. **外键约束**：INSERT/UPDATE 时实时检查引用完整性
6. **聚合执行**：GROUP BY + HAVING，预计算聚合后过滤

## 项目规模

- **代码量**：10,000+ 行
- **测试覆盖**：29 个测试套件，全部通过
- **SQL 覆盖**：MySQL 核心 DDL + DML + SELECT（聚合/连接/子查询）

## 与 MySQL 的区别

| 特性 | MySQL | MiniSQL |
|------|-------|---------|
| 存储引擎 | InnoDB（页+WAL） | 文本文件+B+树 |
| 事务 | ACID + MVCC | 规划中 |
| 并发 | 多线程 + 行锁 | 单线程 |
| 优化器 | 基于代价（CBO） | 规则优化 + 代价优化 |
| SQL 覆盖 | 100% | MySQL 核心 DDL/DML/SELECT |

MiniSQL 专注于展示数据库核心原理，适合学习和面试。

## 贡献

欢迎提交 Issue 和 Pull Request！

## 许可证

MIT License

## 作者

chenzhou

---

**注意**：本项目为学习项目，不建议用于生产环境。
