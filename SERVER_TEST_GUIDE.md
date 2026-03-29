# MiniSQL 服务器测试指南

## 测试脚本说明

### 1. 快速测试 (`test_server_quick.sh`)
**用途**: 快速验证服务器基本功能是否正常
**测试内容**:
- 服务器连接检查
- DDL 操作（CREATE DATABASE/TABLE）
- DML 操作（INSERT/SELECT/UPDATE/DELETE）
- 索引创建
- 聚合查询

**运行方式**:
```bash
chmod +x test_server_quick.sh
./test_server_quick.sh
```

**预计运行时间**: 5-10 秒

---

### 2. 完整功能测试 (`test_server_complete.sh`)
**用途**: 全面测试所有已支持的 SQL 功能
**测试内容**:
- **DDL**: CREATE DATABASE/TABLE/INDEX, DROP, ALTER TABLE, SHOW
- **DML**: INSERT（单行/多行）, SELECT（各种子句）, UPDATE, DELETE
- **查询**: WHERE, ORDER BY, LIMIT, GROUP BY, HAVING, JOIN
- **函数**: 聚合函数（COUNT, SUM, AVG, MIN, MAX）, 字符串函数（UPPER, CONCAT）
- **表达式**: 算术运算, 逻辑运算
- **并发**: 10 个并发客户端同时执行 UPDATE

**运行方式**:
```bash
chmod +x test_server_complete.sh
./test_server_complete.sh
```

**预计运行时间**: 30-60 秒

**预期结果**:
```
总测试数: ~60
通过: ~60
失败: 0
🎉 所有测试通过！
```

---

### 3. 并发压力测试 (`test_server_concurrent.sh`)
**用途**: 测试服务器并发处理能力和数据一致性
**测试内容**:
- **并发读测试**: 50 个客户端 × 100 次查询
- **并发写测试**: 50 个客户端 × 100 次更新
- **混合读写测试**: 50 个客户端 × 100 次操作（70%读，30%写）
- **竞争条件测试**: 10 个客户端 × 100 次计数器更新（验证无数据丢失）
- **长连接测试**: 5 个客户端 × 1000 次操作（验证连接稳定性）

**运行方式**:
```bash
chmod +x test_server_concurrent.sh
./test_server_concurrent.sh
```

**预计运行时间**: 2-5 分钟

**关键指标**:
- 吞吐量（ops/sec）
- 竞争条件测试结果（必须通过）
- 数据一致性验证

---

## 服务器启动

### 编译服务器
```bash
cd build_server
cmake -DBUILD_SERVER=ON ..
make minisql_server -j8
```

### 启动服务器
```bash
# 前台运行（调试用）
./bin/minisql_server

# 后台运行
./bin/minisql_server &

# 查看日志
tail -f data/wal/wal.log
```

### 停止服务器
```bash
# 查找进程
ps aux | grep minisql_server

# 终止进程
kill <pid>

# 或使用 pkill
pkill minisql_server
```

---

## 手动测试

### 使用 nc (netcat)
```bash
# 查询所有数据库
echo "SHOW DATABASES;" | nc localhost 3306

# 创建数据库
echo "CREATE DATABASE testdb;" | nc localhost 3306

# 使用数据库
echo "USE testdb;" | nc localhost 3306

# 创建表
echo "CREATE TABLE users(id INT PRIMARY KEY, name VARCHAR(50));" | nc localhost 3306

# 插入数据
echo "INSERT INTO users VALUES(1, 'Alice');" | nc localhost 3306

# 查询数据
echo "SELECT * FROM users;" | nc localhost 3306
```

### 使用 telnet
```bash
telnet localhost 3306
# 然后输入 SQL 语句，每行以 ; 结尾
```

---

## 协议格式

### 请求格式
```
<SQL>;\n
```

### 响应格式（成功）
```
OK\n
<message>\n
<row_count>\n
<col1>,<col2>,...\n
<row1_col1>,<row1_col2>,...\n
...
```

### 响应格式（错误）
```
ERROR\n
<error_message>\n
```

---

## 故障排查

### 问题 1: 无法连接到服务器
**症状**: `nc: connect to localhost port 3306 (tcp) failed: Connection refused`

**解决方案**:
1. 检查服务器是否运行: `ps aux | grep minisql_server`
2. 检查端口是否被占用: `netstat -tuln | grep 3306`
3. 检查防火墙设置
4. 查看服务器日志: `tail -f build_server/bin/data/wal/wal.log`

---

### 问题 2: 竞争条件测试失败
**症状**: 并发更新后计数器值不等于期望值

**可能原因**:
1. LockManager 未正确实现
2. FileIO 或 BPlusTree 未加锁
3. 事务隔离级别问题

**解决方案**:
1. 检查 `src/server/concurrency/LockManager.cpp`
2. 检查 `src/storage/FileIO.cpp` 中的锁使用
3. 检查 `src/storage/BPlusTree.cpp` 中的锁使用
4. 查看日志中的锁争用信息

---

### 问题 3: 性能测试吞吐量低
**症状**: ops/sec 远低于预期

**可能原因**:
1. 锁粒度过大
2. 未使用索引
3. 文件 I/O 性能问题
4. 线程池配置不当

**解决方案**:
1. 检查锁的持有时间
2. 为常用查询字段创建索引
3. 使用 SSD 存储
4. 调整 ThreadPool 大小（默认 4 个工作线程）

---

### 问题 4: 内存泄漏
**症状**: 长时间运行后内存占用持续增长

**排查方法**:
```bash
# 使用 valgrind 检测内存泄漏
valgrind --leak-check=full --show-leak-kinds=all ./bin/minisql_server

# 使用 top/htop 监控内存使用
top -p $(pidof minisql_server)
```

---

## 性能基准

### 预期性能指标（参考）
| 测试类型 | 客户端数 | 操作数/客户端 | 预期吞吐量 |
|---------|---------|--------------|----------|
| 并发读   | 50      | 100          | > 5000 ops/sec |
| 并发写   | 50      | 100          | > 1000 ops/sec |
| 混合读写 | 50      | 100          | > 2000 ops/sec |
| 竞争条件 | 10      | 100          | 100% 一致性   |

### 影响性能的因素
- CPU 核心数
- 磁盘 I/O 速度
- 网络延迟
- 数据量大小
- 索引使用情况

---

## 扩展测试

### 自定义测试脚本
```bash
#!/bin/bash
for i in {1..1000}; do
    echo "INSERT INTO test VALUES($i, 'Data$i');" | nc localhost 3306
done
```

### 使用 Apache Bench (ab)
```bash
# 需要先创建 HTTP 接口
ab -n 10000 -c 50 http://localhost:8080/query
```

### 使用 JMeter
1. 创建 TCP 取样器
2. 配置服务器地址和端口
3. 添加 SQL 请求
4. 设置并发用户数和循环次数
5. 运行测试

---

## 日志分析

### WAL 日志位置
```
build_server/bin/data/wal/wal.log
```

### 查看实时日志
```bash
tail -f build_server/bin/data/wal/wal.log
```

### 日志级别
- DEBUG: 详细调试信息
- INFO: 一般信息（默认）
- WARN: 警告信息
- ERROR: 错误信息

---

## 下一步

1. **添加更多测试用例**: 覆盖边界条件和异常情况
2. **性能优化**: 根据测试结果优化瓶颈
3. **压力测试**: 测试服务器极限性能
4. **长时间稳定性测试**: 运行 24 小时以上
5. **故障恢复测试**: 模拟服务器崩溃并验证 WAL 恢复

---

## 联系方式

如有问题，请查看项目文档或提交 Issue:
- GitHub: https://github.com/chenzhou0071/SQL_Management_System
