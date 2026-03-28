# MiniSQL 并发控制与事务管理系统设计

**日期:** 2026-03-28
**作者:** chenzhou
**状态:** 设计阶段
**目标:** 为 MiniSQL 添加网络并发和事务支持

---

## 1. 项目概述

### 1.1 目标

为现有的 MiniSQL 单线程数据库添加:
- **Linux服务器**: Reactor模式的TCP服务器,支持多客户端并发访问
- **并发控制**: B+树读写锁、文件操作互斥锁、锁管理器
- **事务管理**: BEGIN/COMMIT/ROLLBACK、读已提交隔离级别、WAL持久化
- **Windows客户端**: Qt GUI界面,通过TCP连接Linux服务器

### 1.2 架构模式

**客户端-服务器(C/S)架构:**
- **Windows**: GUI客户端(Qt界面),只负责界面展示和用户交互
- **Linux**: 服务器(Reactor网络层),负责SQL执行和并发控制

**开发模式:**
- Windows上编写代码(共享文件夹同步到Linux虚拟机)
- Linux虚拟机上编译测试服务器
- Windows GUI直接连接Linux服务器

### 1.3 实现顺序

**阶段1-3: Linux服务器** (优先完成)
1. 并发控制层 (LockManager、B+树锁、FileIO锁)
2. 事务管理层 (TransactionManager、WAL)
3. Reactor网络层 (TcpServer、协议处理)

**阶段4: Windows GUI客户端** (最后完成)
4. Qt界面 + TcpClient

### 1.4 设计原则

- **服务器优先**: 先完成服务器,GUI自然对接
- **渐进式开发**: 分阶段实现,每阶段可测试
- **跨平台**: Windows开发,Linux部署
- **代码分离**: 服务器和客户端模块完全独立

---

## 2. 系统架构

### 2.1 总体架构图

```
┌─────────────────────────────────────────────────────────┐
│          Windows 客户端 (开发环境)                       │
│  ┌──────────────────────────────────────────────────┐  │
│  │         MiniSQL GUI客户端                        │  │
│  │  - Qt界面                                         │  │
│  │  - SQL编辑器                                      │  │
│  │  - TCP客户端                                      │  │
│  └──────────────────────────────────────────────────┘  │
└───────────────────────┬─────────────────────────────────┘
                        │ TCP网络
                        │ 简单文本协议
┌───────────────────────▼─────────────────────────────────┐
│          Linux 服务器 (生产环境)                         │
│  ┌──────────────────────────────────────────────────┐  │
│  │  Reactor网络层                                   │  │
│  │  - EventLoop (epoll)                            │  │
│  │  - 线程池                                        │  │
│  └──────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────┐  │
│  │  事务管理层                                      │  │
│  │  - TransactionManager                            │  │
│  │  - WAL持久化                                     │  │
│  └──────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────┐  │
│  │  并发控制层                                      │  │
│  │  - LockManager                                   │  │
│  │  - B+树读写锁                                    │  │
│  │  - 文件互斥锁                                    │  │
│  └──────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────┐  │
│  │  存储引擎(现有)                                  │  │
│  │  - B+Tree索引 │ SQL解析 │ 执行器                 │  │
│  └──────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

### 2.2 模块依赖关系

```
服务器模块依赖:
┌─────────────────────────────────────┐
│  network (Reactor网络层)             │
│    - Reactor/TcpServer/ThreadPool   │
└──────────────┬──────────────────────┘
               ↓
┌─────────────────────────────────────┐
│  concurrency (并发控制)              │
│    - LockManager                    │
└──────────────┬──────────────────────┘
               ↓
┌─────────────────────────────────────┐
│  transaction (事务管理)              │
│    - TransactionManager/WAL         │
└──────────────┬──────────────────────┘
               ↓
┌─────────────────────────────────────┐
│  executor → storage → parser       │
│    (现有SQL引擎,加锁改造)           │
└─────────────────────────────────────┘

客户端模块依赖:
┌─────────────────────────────────────┐
│  GUI (Qt界面)                       │
│    - MainWindow                     │
└──────────────┬──────────────────────┘
               ↓
┌─────────────────────────────────────┐
│  TcpClient (网络客户端)             │
│    - 连接Linux服务器                 │
└─────────────────────────────────────┘
```

### 2.3 目录结构

```
MiniSQL/
├── src/
│   ├── common/              # 共享: 公共组件
│   ├── parser/              # 共享: SQL解析器
│   ├── optimizer/           # 共享: 查询优化器
│   ├── executor/            # 共享: 查询执行器(需改造加锁)
│   ├── storage/             # 共享: 存储引擎(需加锁)
│   │
│   ├── server/              # Linux服务器专用
│   │   ├── concurrency/     # 并发控制
│   │   │   ├── LockManager.h/cpp
│   │   │   ├── RWLock.h
│   │   │   └── CMakeLists.txt
│   │   ├── transaction/     # 事务管理
│   │   │   ├── TransactionManager.h/cpp
│   │   │   ├── WALManager.h/cpp
│   │   │   ├── Transaction.h
│   │   │   └── CMakeLists.txt
│   │   ├── network/         # Reactor网络
│   │   │   ├── Reactor.h/cpp
│   │   │   ├── TcpServer.h/cpp
│   │   │   ├── TcpConnection.h/cpp
│   │   │   ├── ThreadPool.h/cpp
│   │   │   ├── SqlProtocol.h/cpp
│   │   │   └── CMakeLists.txt
│   │   ├── main_server.cpp  # 服务器入口
│   │   └── CMakeLists.txt
│   │
│   └── client/              # Windows客户端专用
│       ├── network/         # 网络客户端
│       │   ├── TcpClient.h/cpp
│       │   └── CMakeLists.txt
│       ├── gui/             # Qt界面
│       │   ├── MainWindow.h/cpp
│       │   ├── dialogs/     # 对话框
│       │   └── CMakeLists.txt
│       ├── main.cpp         # 客户端入口
│       └── CMakeLists.txt
│
├── tests/
│   ├── server/              # Linux服务器测试
│   │   ├── test_concurrency.cpp
│   │   ├── test_transaction.cpp
│   │   ├── test_network.cpp
│   │   └── CMakeLists.txt
│   └── client/              # Windows客户端测试
│       └── test_tcp_client.cpp
│
└── docs/
```

### 2.4 编译配置

**Windows (GUI客户端):**
```bash
mkdir build && cd build
cmake ..
cmake --build .
# 输出: bin/minisql.exe
```

**Linux (服务器):**
```bash
mkdir build && cd build
cmake -DBUILD_SERVER=ON ..
make
# 输出: bin/minisql_server
```

### 2.5 平台差异处理

**storage模块加锁方案:**

```cpp
// storage/BPlusTree.h
#ifdef __linux__
    std::shared_mutex treeMutex_;  // Linux版本加锁
#endif

// storage/FileIO.h
#ifdef __linux__
    static std::map<std::string, std::mutex> fileMutexes_;
#endif
```

---

## 3. 并发控制层设计

### 3.1 锁粒度选择

**采用中粒度锁策略:**
- 每个表一把读写锁
- 每个索引一把读写锁
- 每个文件一把互斥锁

**理由:**
- 平衡并发度和实现复杂度
- 不同表/索引可真正并发
- 足够展示并发控制原理

### 3.2 LockManager实现

```cpp
class LockManager {
public:
    enum LockType { READ, WRITE };

    // 获取表级锁
    bool acquireTableLock(const std::string& table,
                         LockType type,
                         Transaction* txn);

    // 释放表级锁
    void releaseTableLock(const std::string& table,
                         Transaction* txn);

    // 获取索引锁
    bool acquireIndexLock(const std::string& index,
                         LockType type,
                         Transaction* txn);

    // 释放索引锁
    void releaseIndexLock(const std::string& index,
                         Transaction* txn);

    // 文件操作锁
    void acquireFileLock(const std::string& filePath);
    void releaseFileLock(const std::string& filePath);

private:
    std::map<std::string, std::shared_mutex> tableLocks_;
    std::map<std::string, std::shared_mutex> indexLocks_;
    std::map<std::string, std::mutex> fileLocks_;
};
```

### 3.3 死锁处理

**超时机制:**
- 锁获取超时: 5秒
- 超时后抛出异常,事务回滚
- 简单有效,适合学习项目

### 3.4 B+树加锁

```cpp
class BPlusTree {
private:
    mutable std::shared_mutex treeMutex_;

public:
    // 读操作 - 共享锁
    int find(int64_t key) {
        std::shared_lock<std::shared_mutex> lock(treeMutex_);
        // 原有逻辑
    }

    std::vector<int> rangeSearch(int64_t min, int64_t max) {
        std::shared_lock<std::shared_mutex> lock(treeMutex_);
        // 原有逻辑
    }

    // 写操作 - 独占锁
    bool insert(int64_t key, int rowId) {
        std::unique_lock<std::shared_mutex> lock(treeMutex_);
        // 原有逻辑
    }

    bool remove(int64_t key) {
        std::unique_lock<std::shared_mutex> lock(treeMutex_);
        // 原有逻辑
    }
};
```

### 3.5 FileIO加锁

```cpp
class FileIO {
private:
    static std::map<std::string, std::mutex> fileMutexes_;
    static std::mutex globalMutex_;

public:
    static bool writeToFile(const std::string& path,
                           const std::string& content) {
        std::lock_guard<std::mutex> lock(getFileMutex(path));
        // 原有逻辑
    }

    static std::string readFromFile(const std::string& path) {
        std::lock_guard<std::mutex> lock(getFileMutex(path));
        // 原有逻辑
    }

private:
    static std::mutex& getFileMutex(const std::string& path) {
        std::lock_guard<std::mutex> lock(globalMutex_);
        if (fileMutexes_.find(path) == fileMutexes_.end()) {
            fileMutexes_[path] = std::mutex();
        }
        return fileMutexes_[path];
    }
};
```

---

## 4. 事务管理层设计

### 4.1 事务特性

**隔离级别:** Read Committed (读已提交)
- 避免脏读
- 允许不可重复读
- 实现难度适中

**支持的操作:** DML (SELECT/INSERT/UPDATE/DELETE)
- DDL操作自动提交之前的未提交事务
- 符合主流数据库行为

### 4.2 Transaction实现

```cpp
class Transaction {
public:
    enum State { ACTIVE, COMMITTED, ABORTED };
    enum IsolationLevel { READ_COMMITTED };

    uint64_t txnId_;
    IsolationLevel isolationLevel_;
    State state_;

    // 操作日志
    std::vector<Operation> operations_;

    // 持有的锁
    std::vector<Lock> heldLocks_;

    // 记录操作
    void recordOperation(OpType type,
                        const std::string& table,
                        const std::vector<Value>& data);
};

class TransactionManager {
public:
    // 开始事务
    Transaction* beginTransaction();

    // 提交事务
    bool commit(Transaction* txn);

    // 回滚事务
    bool rollback(Transaction* txn);

    // 获取当前线程的事务
    Transaction* getCurrentTransaction();

private:
    std::atomic<uint64_t> nextTxnId_;
    std::mutex txnMapMutex_;
    std::unordered_map<uint64_t, Transaction*> activeTransactions_;

    WALManager* wal_;
};
```

### 4.3 WAL持久化

**WAL日志格式:**
```
[LSN:8字节][事务ID:8字节][日志类型:1字节][数据长度:4字节][数据]
```

**日志类型:**
- BEGIN: 开始事务
- INSERT: 插入记录
- UPDATE: 更新记录
- DELETE: 删除记录
- COMMIT: 提交事务

**WALManager实现:**

```cpp
class WALManager {
public:
    // 写日志
    void writeLog(uint64_t txnId,
                  LogType type,
                  const std::string& data);

    // 持久化(fsync)
    void flush();

    // 崩溃恢复
    void recover();

private:
    std::ofstream walFile_;
    std::mutex walMutex_;
};
```

### 4.4 提交流程

```
1. 写WAL日志 (fsync到磁盘)
2. 执行操作 (修改B+树和文件)
3. 标记事务为COMMITTED
4. 释放所有锁
5. 返回成功
```

### 4.5 回滚流程

```
1. 释放所有锁
2. 从后往前撤销操作
   - INSERT: 删除记录
   - DELETE: 恢复记录
   - UPDATE: 恢复旧值
3. 标记事务为ABORTED
4. 从活动事务列表移除
```

---

## 5. 网络层设计

### 5.1 Reactor模式

**选择:** 单Reactor多线程

```
主线程:
  创建epoll → 绑定端口 → accept连接 → 注册读事件

Reactor线程:
  epoll_wait → 有事件:
    - 新连接: accept → 创建TcpConnection
    - 有数据: 读取SQL → 提交到工作线程池

工作线程:
  解析SQL → 加锁 → 执行 → 写WAL → 返回结果 → 释放锁
```

**优点:**
- 分离I/O和计算
- Reactor只负责I/O,业务逻辑在工作线程
- 配合锁机制完美工作
- 性能远超单线程版本

### 5.2 Reactor实现

```cpp
class Reactor {
public:
    Reactor(int port, int threadCount = 4);
    void start();
    void stop();

private:
    int epollFd_;
    int listenFd_;
    std::vector<std::thread> threadPool_;

    void eventLoop();
    void handleAccept();
    void handleRead(int fd);
    void handleWrite(int fd, const std::string& response);
};

class TcpConnection {
public:
    int fd_;
    std::string buffer_;
    Transaction* currentTxn_;

    std::string extractCommand();
};
```

### 5.3 线程池实现

```cpp
class ThreadPool {
public:
    ThreadPool(size_t threadCount);
    ~ThreadPool();

    // 提交任务
    template<typename F>
    auto submit(F&& f) -> std::future<decltype(f())>;

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queueMutex_;
    std::condition_variable condition_;
    bool stop_;
};
```

### 5.4 协议设计

**协议格式:** 简单文本协议

**请求:**
```
SQL语句;\n
```

**响应:**
```
状态码(OK/ERROR)\n
消息\n
[行数]\n
列1,列2,...\n
数据1,数据2,...\n
...
```

**示例:**
```
客户端: SELECT * FROM users;\n
服务端: OK\nQuery returned 2 rows\n2\nid,name,dept_id\n1,Alice,1\n2,Bob,2\n
```

**SqlProtocol实现:**

```cpp
class SqlProtocol {
public:
    struct Request {
        std::string sql;
        uint64_t txnId;  // 0表示无事务
    };

    struct Response {
        bool success;
        std::string message;
        std::vector<std::vector<std::string>> rows;
    };

    static Request parse(const std::string& data);
    static std::string buildResponse(const Response& res);
};
```

---

## 6. GUI客户端设计

### 6.1 主界面布局

```
┌─────────────────────────────────────────────────────────────────┐
│  MiniSQL  [服务器: 192.168.1.100:3306]        [主题] [刷新]     │
├─────────────────────────────────────────────────────────────────┤
│  [连接] [断开] [新建库] [新建表] [插入] [修改] [删除] [查询]     │
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
│  SQL终端区                                                      │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │ mysql> SELECT * FROM users WHERE age > 18;                 ││
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

### 6.2 交互方式

1. **手动输入**: 在终端区输入SQL命令,按Enter执行
2. **按钮操作**: 点击按钮 → 弹出对话框填写参数 → 自动生成SQL到终端执行
3. **数据编辑**: 点击表格单元格直接修改 → 自动生成UPDATE语句执行
4. **服务器连接**: 点击"连接"按钮 → 输入服务器IP和端口 → 连接到Linux服务器

### 6.3 连接对话框

```
┌─────────────────────────────────────────────────────┐
│              连接到服务器                            │
├─────────────────────────────────────────────────────┤
│  服务器地址: [192.168.1.100              ]           │
│  端口:       [3306                       ]           │
│                                                     │
│  状态:       [未连接]                               │
│                                                     │
│              [取消]              [连接]             │
└─────────────────────────────────────────────────────┘
```

### 6.4 工具栏功能

所有按钮均为弹窗模式,点击按钮弹出对话框填写参数,确认后自动生成SQL到终端执行。

| 按钮 | 功能 | 弹窗内容 |
|------|------|----------|
| 连接 | 连接到服务器 | 服务器IP、端口 |
| 断开 | 断开当前连接 | 确认对话框 |
| 新建库 | 创建数据库 | 输入库名 |
| 新建表 | 创建表 | 表名、字段列表(名称、类型、约束) |
| 插入 | 插入数据 | 各字段输入框(显示当前表结构) |
| 修改 | 更新数据 | SET字段=新值 WHERE 条件 |
| 删除 | 删除数据 | WHERE 条件 |
| 查询 | SELECT查询 | 选择字段、WHERE条件、ORDER BY、LIMIT等 |
| 刷新 | 重新加载当前表数据 | 自动执行SELECT * FROM 当前表 |
| 主题 | 切换亮色/暗色主题 | 自动切换 |

### 6.5 弹窗示例

**查询弹窗**:
```
┌─────────────────────────────────────────────────────┐
│              查询 - users                            │
├─────────────────────────────────────────────────────┤
│  字段选择:                                          │
│  ☑ id   ☑ name   ☑ age   ☑ email                 │
│                                                     │
│  WHERE条件:                                         │
│  字段: [age    ▼]  操作符: [>  ▼]  值: [18      ]  │
│                                                     │
│  ORDER BY:  [id ▼]  [ASC ▼]                        │
│  LIMIT:     [10      ]                              │
│                                                     │
│            [取消]              [执行查询]           │
└─────────────────────────────────────────────────────┘
```

### 6.6 功能模块实现

**MainWindow核心类:**

```cpp
class MainWindow : public QMainWindow {
private:
    std::unique_ptr<NetworkClient> dbClient_;
    QTableView* resultTable_;
    QTextEdit* sqlTerminal_;
    QTreeWidget* databaseTree_;

    // 服务器连接
    void connectToServer();
    void disconnectFromServer();

    // SQL执行
    void executeSQL(const std::string& sql);
    void displayResult(const Result& result);

    // 表格操作
    void createDatabase();
    void createTable();
    void insertData();
    void updateData();
    void deleteData();
    void queryData();

    // UI更新
    void refreshDatabaseList();
    void refreshTableData();
    void switchTheme();
};
```

**TcpClient实现:**

```cpp
class TcpClient {
public:
    TcpClient(const std::string& host, int port);
    ~TcpClient();

    // 连接服务器
    bool connect();

    // 发送SQL
    bool sendQuery(const std::string& sql);

    // 接收响应
    SqlProtocol::Response receiveResponse();

    // 断开连接
    void disconnect();

private:
    int socketFd_;
    std::string host_;
    int port_;
};
```

---

## 7. 错误处理和恢复

### 7.1 并发错误

```cpp
class ConcurrentError : public std::runtime_error {
public:
    enum ErrorType {
        LOCK_TIMEOUT,
        DEADLOCK,
        TXN_CONFLICT,
        WAL_WRITE_FAILED
    };

    ErrorType type_;
    std::string details_;
};
```

### 7.2 网络错误处理

```cpp
class TcpConnection {
    void handleError(const std::string& error) {
        // 自动回滚活动事务
        if (currentTxn_) {
            TransactionManager::getInstance()
                .rollback(currentTxn_);
        }

        // 发送错误响应
        sendResponse("ERROR\n" + error + "\n");

        // 关闭连接
        close(fd_);
    }
};
```

### 7.3 WAL恢复

```cpp
void WALManager::recover() {
    // 1. 读取WAL文件
    auto logs = readWalFile();

    // 2. 重做已提交的事务
    for (auto& log : logs) {
        if (log.type == LogType::COMMIT) {
            redoTransaction(log.txnId);
        }
    }

    // 3. 清理WAL
    truncateWal();
}
```

---

## 8. 测试策略

### 8.1 单元测试

- `test_lock_manager.cpp`: 锁机制测试
- `test_transaction.cpp`: 事务管理测试
- `test_wal.cpp`: WAL持久化测试

### 8.2 并发测试

- `test_concurrency.cpp`: 多线程并发测试
- `test_transaction_isolation.cpp`: 隔离级别测试

### 8.3 网络测试

- `test_network.cpp`: TCP服务器测试
- 压力测试: 50个并发客户端

### 8.4 集成测试

- 完整的客户端-服务器通信测试
- 崩溃恢复测试

---

## 9. 项目目录结构

```
MiniSQL/
├── src/
│   ├─�� common/              # 现有:公共组件
│   ├── storage/             # 现有:存储引擎(需要加锁)
│   ├── parser/              # 现有:SQL解析器
│   ├── optimizer/           # 现有:查询优化器
│   ├── executor/            # 现有:查询执行器
│   ├── transaction/         # 事务管理(完整实现)
│   ├── concurrency/         # 新增:并发控制
│   ├── network/             # 新增:网络层
│   │   ├── TcpClient.h/cpp      # GUI用
│   │   ├── Reactor.h/cpp
│   │   ├── TcpServer.h/cpp
│   │   └── CMakeLists.txt
│   ├── application/         # GUI层
│   │   ├── MainWindow.h/cpp
│   │   └── CMakeLists.txt
│   ├── main.cpp             # GUI入口(Windows)
│   └── main_server.cpp      # Server入口(Linux)
├── tests/                   # 测试
├── data/
│   ├── wal/                 # WAL日志
│   └── databases/           # 数据库文件
└── CMakeLists.txt           # 支持BUILD_SERVER选项
```

---

## 10. 实现计划

### 阶段1: 并发控制层 (1-2周)

**目标:** 为存储引擎添加线程安全的并发控制

**任务清单:**
1. 实现RWLock、LockManager
2. 修改BPlusTree添加读写锁
3. 修改FileIO添加文件互斥锁
4. 修改TableManager集成锁管理
5. 编写并发测试

**验证标准:**
- 多线程并发读写B+树不崩溃
- 文件操作线程安全
- 锁超时机制正常工作

### 阶段2: 事务管理 (1-2周)

**目标:** 实现完整的事务管理功能和WAL持久化

**任务清单:**
1. 实现Transaction、TransactionManager
2. 实现WALManager
3. 修改执行器集成事务
4. 实现BEGIN/COMMIT/ROLLBACK语法
5. 编写事务测试

**验证标准:**
- BEGIN/COMMIT/ROLLBACK正常工作
- 崩溃后数据能恢复(WAL)
- 并发事务隔离级别正确

### 阶段3: 网络层 (2-3周)

**目标:** 实现Reactor模式的TCP服务器

**任务清单:**
1. 实现SqlProtocol、TcpConnection
2. 实现ThreadPool、Reactor
3. 实现TcpServer、main_server.cpp
4. 完善NetworkClient实现
5. 编写网络测试

**验证标准:**
- telnet能连接并执行SQL
- 多客户端并发访问不崩溃
- 长连接稳定

---

### 阶段4: GUI客户端 (2-3周)

**目标:** 实现Windows GUI客户端,连接到Linux服务器

**任务清单:**
1. 设计MainWindow布局
   - 连接对话框(服务器IP、端口)
   - SQL编辑器(多行输入)
   - 结果显示表格
   - 表管理界面
2. 实现NetworkClient
   - TCP连接管理
   - 异步通信
3. 实现功能模块
   - SQL执行和结果显示
   - 表结构查看
   - 查询历史
4. UI美化

**验证标准:**
- GUI可以连接Linux服务器
- SQL执行结果显示正确
- 错误处理友好

---

### 阶段5: 集成测试 (1周)

**目标:** 端到端测试和性能优化

**任务清单:**
1. GUI连接Linux服务器测试
2. 并发压力测试

**验证标准:**
- 50个并发客户端稳定运行
- 事务ACID特性完整
- 崩溃恢复100%成功
- 性能达到预期目标

---

**总时间估算:** 7-10周

**关键里程碑:**
- 3周后: 并发控制层完成
- 5周后: 事务管理完成
- 8周后: 网络服务器上线(telnet可测试)
- 10周后: GUI客户端完成,系统完整可用

---

## 11. 编译和部署

### Windows (GUI客户端)

```bash
mkdir build && cd build
cmake ..
cmake --build .
# 输出: bin/minisql.exe
```

### Linux (服务器)

```bash
mkdir build && cd build
cmake -DBUILD_SERVER=ON ..
make
# 输出: bin/minisql_server
```

---

## 12. 风险和缓解

| 风险 | 缓解措施 |
|------|----------|
| Reactor实现复杂 | 先实现简单版本,逐步优化 |
| 锁粒度难以把握 | 从粗粒度开始,逐步细化 |
| WAL恢复复杂 | 先实现内存事务,再加持久化 |
| 跨平台问题 | Linux上尽早测试 |
| 性能瓶颈 | 使用性能分析工具定位 |

---

## 13. 总结

本设计为MiniSQL添加了完整的并发控制和事务管理功能,采用客户端-服务器架构,支持Windows GUI客户端和Linux服务器部署。设计遵循学习导向,完整展示数据库核心原理,同时保持实现可行性。

**关键特性:**
- ✅ Reactor网络模型
- ✅ 中粒度读写锁
- ✅ Read Committed隔离级别
- ✅ WAL持久化
- ✅ 跨平台支持

**下一步:** 等待用户确认设计,开始编写实现计划。
