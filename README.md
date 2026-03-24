# MiniSQL - 轻量级关系型数据库管理系统

## 项目简介

MiniSQL 是一款基于 C++17 与 Qt5/6 开发的轻量级关系型数据库管理系统（DBMS），从零实现 MySQL 核心语法，重点展示数据库底层原理和工程化实现能力。

## 技术栈

- **编程语言**：C++17
- **GUI框架**：Qt 5.15/6.x
- **构建工具**：CMake
- **目标平台**：Windows

## 核心功能

- ✅ **完整的 SQL 解析**：词法分析、语法分析、语义检查
- ✅ **查询优化器**：基于规则的优化（RBO）、索引选择
- ✅ **B+树索引**：主键索引、唯一索引、复合索引
- ✅ **基础事务**：原子性、一致性、持久性（ACID）
- ✅ **进阶特性**：视图、存储过程、触发器
- ✅ **双模式交互**：GUI 图形界面 + CLI 终端模式
- ✅ **数据持久化**：文本文件存储，自动保存

## 项目结构

```
MiniSQL/
├── docs/               # 文档（仅上传 docs/plans/）
│   └── plans/         # 设计文档和实现计划
├── src/               # 源代码
│   ├── application/   # 应用层（GUI/CLI）
│   ├── parser/        # SQL 解析层
│   ├── optimizer/     # 查询优化器
│   ├── executor/      # SQL 执行器
│   ├── storage/       # 存储引擎
│   ├── transaction/   # 事务管理
│   └── common/        # 公共组件
├── data/              # 数据目录（运行时生成）
├── tests/             # 测试代码
└── CMakeLists.txt     # CMake 构建配置
```

## 设计文档

- [设计文档](docs/plans/2026-03-19-MiniSQL-Design.md) - 系统架构和模块设计
- [实现计划](docs/plans/2026-03-19-MiniSQL-Implementation-Plan.md) - 开发路线图
- [简历写法](docs/plans/2026-03-19-MiniSQL-Resume.md) - 项目简历描述

## 快速开始

### 环境要求

- Qt 5.15 或 Qt 6.x
- CMake 3.15+
- C++17 编译器（MSVC/GCC/Clang）

### 构建项目

```bash
# 克隆仓库
git clone <repository-url>
cd MiniSQL

# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译
cmake --build .
```

### 运行

```bash
# Windows
.\MiniSQL.exe

# Linux/Mac
./MiniSQL
```

## 使用示例

### GUI 模式

启动后进入图形界面：
- 左侧：数据库/表导航
- 中间：数据展示（点击单元格可直接编辑）
- 下方：SQL 终端（输入 SQL 命令）

### CLI 模式

```sql
-- 创建数据库
mysql> CREATE DATABASE test_db;

-- 使用数据库
mysql> USE test_db;

-- 创建表
mysql> CREATE TABLE users (
    ->   id INT PRIMARY KEY,
    ->   name VARCHAR(50) NOT NULL,
    ->   age INT DEFAULT 0
    -> );

-- 插入数据
mysql> INSERT INTO users VALUES (1, 'Alice', 20);
mysql> INSERT INTO users VALUES (2, 'Bob', 25);

-- 查询数据
mysql> SELECT * FROM users WHERE age > 18;

-- 创建索引
mysql> CREATE INDEX idx_age ON users(age);

-- 查看执行计划
mysql> EXPLAIN SELECT * FROM users WHERE age > 18;
```

## 技术亮点

1. **SQL 解析器**：三阶段分析（词法/语法/语义），递归下降算法
2. **B+树索引**：手动实现，支持高效查找和范围查询
3. **查询优化**：基于规则的优化器，自动选择最优索引
4. **事务支持**：操作日志实现原子性回滚
5. **全栈开发**：从存储引擎到 GUI 界面完整实现

## 项目规模

- **代码量**：约 7000 行
- **开发周期**：6 个月
- **SQL 覆盖**：40% MySQL 核心语法

## 开发路线

项目分为 11 个阶段，按顺序实现：

1. 基础框架（1-2周）
2. 存储引擎（2-3周）
3. SQL 解析（3-4周）
4. 执行器（2-3周）
5. 优化器（1-2周）
6. 基础功能（2-3周）
7. 高级查询（2-3周）
8. 事务（1-2周）
9. 进阶功能（2-3周）
10. GUI 界面（2-3周）
11. 测试和优化（1-2周）

详见 [实现计划](docs/plans/2026-03-19-MiniSQL-Implementation-Plan.md)

## 与 MySQL 的区别

| 特性 | MySQL | MiniSQL |
|------|-------|---------|
| 存储引擎 | InnoDB（页+WAL） | 文本文件+B+树 |
| 事务 | ACID + MVCC | 基础事务（原子性回滚） |
| 并发 | 多线程 + 行锁 | 单线程 |
| 优化器 | 基于代价（CBO） | 基于规则（RBO） |
| SQL 覆盖 | 100% | 40% 核心语法 |

MiniSQL 专注于展示数据库核心原理，适合学习和面试。

## 贡献

欢迎提交 Issue 和 Pull Request！

## 许可证

MIT License

## 作者

chenzhou

---

**注意**：本项目为学习项目，不建议用于生产环境。
