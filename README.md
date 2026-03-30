# MiniSQL - 轻量级关系型数据库管理系统

## 项目简介

MiniSQL 是一款基于 C++17 开发的轻量级关系型数据库管理系统（C/S 架构），从零实现 MySQL 核心语法，支持高并发连接和 GUI 客户端管理。

## 技术栈

| 组件 | 技术 |
|------|------|
| **服务器** | C++17, Linux, TCP/IP, epoll, Reactor, 线程池, B+树 |
| **客户端** | C++17, Qt 6.5.3, CMake, MinGW |
| **构建** | CMake 3.15+ |

## 核心功能

### 服务器端
- ✅ **完整 SQL 解析**：词法分析（Lexer）、语法分析（Parser）、语义检查
- ✅ **查询优化器**：规则优化（RBO）、代价优化（CBO）、索引选择
- ✅ **B+树索引**：主键索引、唯一索引、复合索引、持久化
- ✅ **Volcano 执行模型**：TableScan、Filter、Project、Join、Aggregate、Sort、Limit 等算子
- ✅ **高并发服务**：Reactor + epoll 事件驱动 + 线程池，支持多客户端同时访问
- ✅ **事务支持**：WAL 预写日志、表级锁、会话级数据库隔离
- ✅ **高级 SELECT**：INNER/LEFT/RIGHT JOIN、FROM 子查询、GROUP BY + HAVING
- ✅ **外键约束**：INSERT/UPDATE 时级联引用检查
- ✅ **数据持久化**：JSON 元数据 + CSV 数据 + JSON 索引文件

### 客户端
- ✅ **Qt GUI 界面**：IP/端口连接、SQL 执行、表格结果展示
- ✅ **连接管理**：连接状态提示、错误弹窗
- ✅ **结果格式化**：MySQL 风格表格输出

## 已完成阶段

| Phase | 内容 | 状态 |
|-------|------|------|
| Phase 1 | 基础框架（数据类型、值系统、错误处理、日志） | ✅ |
| Phase 2 | 存储引擎（B+树索引、表管理、文件操作） | ✅ |
| Phase 3 | SQL 解析器（词法/语法/语义分析、函数调用、IN/EXISTS 子查询） | ✅ |
| Phase 4 | 查询执行器（Volcano 模型、全部执行算子、DML/DDL） | ✅ |
| Phase 5 | 查询优化器（统计信息、索引选择、查询重写、代价模型） | ✅ |
| Phase 6 | 基础功能集成（优化器入口、外键约束、子查询执行） | ✅ |
| Phase 7 | 高级 SELECT（JOIN、FROM 子查询、HAVING、索引选择恢复） | ✅ |
| Phase 8 | Linux 服务器（Reactor + 线程池 + 事务 + WAL + 并发控制） | ✅ |
| Phase 9 | 服务器并发修复（会话隔离、Result 内存管理） | ✅ |
| Phase 10 | Windows GUI 客户端（Qt 连接管理 + SQL 执行 + 结果展示） | ✅ |

## 项目结构

```
MiniSQL/
├── src/
│   ├── common/        # 公共组件（数据类型、错误处理、日志、Result<T>）
│   ├── storage/       # 存储引擎（B+树、表管理、文件操作）
│   ├── parser/        # SQL 解析层（Lexer、Parser、AST、SemanticAnalyzer）
│   ├── optimizer/     # 查询优化器（统计信息、索引选择、计划生成）
│   ├── executor/      # SQL 执行器（算子体系、表达式求值、DML/DDL）
│   ├── client/        # Windows GUI 客户端
│   │   ├── main.cpp           # 程序入口
│   │   ├── mainwindow.h/cpp   # 主窗口
│   │   └── tcpclient.h/cpp    # TCP 连接
│   └── server/        # Linux 服务器
│       ├── concurrency/  # 并发控制（RWLock、LockManager）
│       ├── transaction/ # 事务管理（Transaction、WAL、TransactionManager）
│       └── network/     # 网络层（Reactor、ThreadPool、SqlProtocol）
├── tests/             # 测试代码（29 个测试套件）
├── docs/             # 设计文档
└── CMakeLists.txt    # CMake 构建配置
```

## 快速开始

### 环境要求

**通用**：
- CMake 3.15+
- C++17 编译器

**Windows GUI 客户端**：
- Qt 6.5.3 (MinGW)
- MinGW 编译器

**Linux 服务器**：
- Linux (Ubuntu 20.04+)
- pthread

### 构建与运行

#### Windows GUI 客户端

```bash
# 克隆项目
git clone https://github.com/chenzhou0071/SQL_Management_System.git
cd SQL_Management_System

# 编译
mkdir build && cd build
cmake ..
ninja minisql-client

# 运行（先启动 Linux 服务器）
./bin/minisql-client.exe
```

#### Linux 服务器

```bash
# 克隆项目
git clone https://github.com/chenzhou0071/SQL_Management_System.git
cd SQL_Management_System

# 编译
mkdir build && cd build
cmake .. -DBUILD_SERVER=ON
make -j4

# 启动服务器
./bin/minisql_server
```

### 测试 SQL

```sql
-- 创建数据库和表
CREATE DATABASE test_db;
USE test_db;
CREATE TABLE users (id INT, name VARCHAR(50), age INT);
CREATE TABLE orders (id INT, user_id INT, amount INT);

-- 创建索引
CREATE INDEX idx_age ON users(age);

-- 插入数据
INSERT INTO users VALUES (1, 'Alice', 25);
INSERT INTO users VALUES (2, 'Bob', 30);
INSERT INTO orders VALUES (1, 1, 100);
INSERT INTO orders VALUES (2, 1, 200);

-- 查询
SELECT * FROM users WHERE age > 20;
SELECT u.name, o.amount FROM users u JOIN orders o ON u.id = o.user_id;

-- GROUP BY
SELECT COUNT(*), age FROM users GROUP BY age HAVING COUNT(*) > 0;

-- 查看执行计划
EXPLAIN SELECT * FROM users WHERE age > 18;
```

## 客户端界面

```
┌─────────────────────────────────────────────────────────────┐
│  MiniSQL Client                                             │
├─────────────────────────────────────────────────────────────┤
│  IP: [192.168.1.100]  Port: [3306]  [Connect]             │
├─────────────────────────────────────────────────────────────┤
│  SQL> SELECT * FROM users;                                 │
│  +----+-------+-----+                                     │
│  | id | name  | age |                                     │
│  +----+-------+-----+                                     │
│  | 1  | Alice | 25  |                                     │
│  | 2  | Bob   | 30  |                                     │
│  +----+-------+-----+                                     │
│  2 rows in set                                             │
│                                                             │
│  SQL> _                                                    │
└─────────────────────────────────────────────────────────────┘
```

## 技术亮点

1. **C/S 架构**：Windows Qt 客户端 + Linux 高并发服务器
2. **Reactor 事件驱动**：epoll 非阻塞 I/O，支持 100+ 并发连接
3. **线程池执行**：4 线程池处理 SQL，~900 ops/sec 吞吐量
4. **会话隔离**：每个连接独立的数据库上下文
5. **B+树索引**：手动实现，支持范围查询和持久化
6. **WAL 日志**：崩溃恢复保障数据安全

## 项目规模

- **代码量**：15,000+ 行
- **测试覆盖**：29 个测试套件，全部通过
- **SQL 覆盖**：MySQL 核心 DDL + DML + SELECT（聚合/连接/子查询）
- **并发性能**：10 客户端 × 20 操作 = ~900 ops/sec

## 与 MySQL 的区别

| 特性 | MySQL | MiniSQL |
|------|-------|---------|
| 存储引擎 | InnoDB（页+WAL） | 文本文件+B+树 |
| 事务 | ACID + MVCC | WAL + 表级锁 |
| 并发 | 多线程 + 行锁 | Reactor + 线程池 + 会话隔离 |
| 优化器 | 基于代价（CBO） | 规则优化 + 代价优化 |
| SQL 覆盖 | 100% | MySQL 核心 DDL/DML/SELECT |

MiniSQL 专注于展示数据库核心原理，适合学习和面试。

## 贡献

欢迎提交 Issue 和 Pull Request！

## 许可证

MIT License

## 作者

chenzhou

## GitHub

https://github.com/chenzhou0071/SQL_Management_System

---

**注意**：本项目为学习项目，不建议用于生产环境。
