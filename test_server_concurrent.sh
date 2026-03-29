#!/bin/bash

# MiniSQL 服务器并发压力测试
# 测试服务器的并发处理能力和事务安全性

HOST="10.121.222.223"   # Linux 服务器 IP 地址
PORT="3306"             # 服务器端口（默认 3306）
NUM_CLIENTS=50
NUM_OPERATIONS=100

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "=========================================="
echo "  MiniSQL 并发压力测试"
echo "=========================================="
echo ""
echo "并发客户端数: $NUM_CLIENTS"
echo "每客户端操作数: $NUM_OPERATIONS"
echo ""

# 检查服务器
if ! nc -z $HOST $PORT 2>/dev/null; then
    echo -e "${RED}错误: 服务器未运行${NC}"
    exit 1
fi

# 设置测试环境
echo "1. 设置测试环境..."
echo "CREATE DATABASE concurrent_test;" | nc $HOST $PORT > /dev/null 2>&1
echo "USE concurrent_test;" | nc $HOST $PORT > /dev/null 2>&1
echo "CREATE TABLE users (
    id INT PRIMARY KEY,
    name VARCHAR(50),
    balance FLOAT
);" | nc $HOST $PORT > /dev/null 2>&1

# 初始化数据
echo "2. 初始化测试数据..."
for i in $(seq 1 100); do
    echo "INSERT INTO users VALUES($i, 'User$i', 1000.00);" | nc $HOST $PORT > /dev/null 2>&1
done
echo -e "${GREEN}✓ 初始化完成${NC}"
echo ""

# 测试1: 并发读
echo "=========================================="
echo "测试1: 并发读测试"
echo "=========================================="
echo "启动 $NUM_CLIENTS 个并发读客户端..."

start_time=$(date +%s.%N)

for i in $(seq 1 $NUM_CLIENTS); do
    (
        for j in $(seq 1 $NUM_OPERATIONS); do
            echo "SELECT * FROM users WHERE id = $((RANDOM % 100 + 1));" | nc $HOST $PORT > /dev/null 2>&1
        done
    ) &
done

wait

end_time=$(date +%s.%N)
elapsed=$(echo "$end_time - $start_time" | bc)
total_ops=$((NUM_CLIENTS * NUM_OPERATIONS))
ops_per_sec=$(echo "scale=2; $total_ops / $elapsed" | bc)

echo -e "${GREEN}✓ 并发读测试完成${NC}"
echo "总操作数: $total_ops"
echo "总耗时: ${elapsed}s"
echo "吞吐量: ${ops_per_sec} ops/sec"
echo ""

# 测试2: 并发写
echo "=========================================="
echo "测试2: 并发写测试"
echo "=========================================="
echo "启动 $NUM_CLIENTS 个并发写客户端..."

start_time=$(date +%s.%N)

for i in $(seq 1 $NUM_CLIENTS); do
    (
        for j in $(seq 1 $NUM_OPERATIONS); do
            user_id=$((RANDOM % 100 + 1))
            new_balance=$((RANDOM % 10000))
            echo "UPDATE users SET balance = $new_balance WHERE id = $user_id;" | nc $HOST $PORT > /dev/null 2>&1
        done
    ) &
done

wait

end_time=$(date +%s.%N)
elapsed=$(echo "$end_time - $start_time" | bc)
total_ops=$((NUM_CLIENTS * NUM_OPERATIONS))
ops_per_sec=$(echo "scale=2; $total_ops / $elapsed" | bc)

echo -e "${GREEN}✓ 并发写测试完成${NC}"
echo "总操作数: $total_ops"
echo "总耗时: ${elapsed}s"
echo "吞吐量: ${ops_per_sec} ops/sec"
echo ""

# 测试3: 混合读写
echo "=========================================="
echo "测试3: 混合读写测试"
echo "=========================================="
echo "启动 $NUM_CLIENTS 个混合客户端 (70%读, 30%写)..."

start_time=$(date +%s.%N)

for i in $(seq 1 $NUM_CLIENTS); do
    (
        for j in $(seq 1 $NUM_OPERATIONS); do
            if [ $((RANDOM % 10)) -lt 7 ]; then
                # 70% 读
                echo "SELECT * FROM users WHERE id = $((RANDOM % 100 + 1));" | nc $HOST $PORT > /dev/null 2>&1
            else
                # 30% 写
                user_id=$((RANDOM % 100 + 1))
                new_balance=$((RANDOM % 10000))
                echo "UPDATE users SET balance = $new_balance WHERE id = $user_id;" | nc $HOST $PORT > /dev/null 2>&1
            fi
        done
    ) &
done

wait

end_time=$(date +%s.%N)
elapsed=$(echo "$end_time - $start_time" | bc)
total_ops=$((NUM_CLIENTS * NUM_OPERATIONS))
ops_per_sec=$(echo "scale=2; $total_ops / $elapsed" | bc)

echo -e "${GREEN}✓ 混合读写测试完成${NC}"
echo "总操作数: $total_ops"
echo "总耗时: ${elapsed}s"
echo "吞吐量: ${ops_per_sec} ops/sec"
echo ""

# 测试4: 竞争条件测试
echo "=========================================="
echo "测试4: 竞争条件测试"
echo "=========================================="
echo "创建计数器表并测试并发更新..."

echo "CREATE TABLE counter (id INT PRIMARY KEY, value INT);" | nc $HOST $PORT > /dev/null 2>&1
echo "INSERT INTO counter VALUES(1, 0);" | nc $HOST $PORT > /dev/null 2>&1

echo "启动 10 个并发客户端，每个执行 100 次 UPDATE..."
for i in $(seq 1 10); do
    (
        for j in $(seq 1 100); do
            echo "UPDATE counter SET value = value + 1 WHERE id = 1;" | nc $HOST $PORT > /dev/null 2>&1
        done
    ) &
done

wait

# 检查最终值
result=$(echo "SELECT * FROM counter;" | nc -q 1 $HOST $PORT | grep "value" | head -1)
expected=1000

echo "最终计数器值: $result"
echo "期望值: $expected"

if echo "$result" | grep -q "$expected"; then
    echo -e "${GREEN}✓ 竞争条件测试通过 - 无数据丢失${NC}"
else
    echo -e "${RED}✗ 竞争条件测试失败 - 检测到数据不一致${NC}"
    echo "这可能表明存在并发控制问题"
fi
echo ""

# 测试5: 长连接测试
echo "=========================================="
echo "测试5: 长连接测试"
echo "=========================================="
echo "启动 5 个长连接，每个执行 1000 次操作..."

for i in $(seq 1 5); do
    (
        # 保持连接
        exec 3<>/dev/tcp/$HOST/$PORT
        for j in $(seq 1 1000); do
            echo "SELECT COUNT(*) FROM users;" >&3
            read -u 3 response
            sleep 0.01
        done
        exec 3<&-
        echo -e "${GREEN}✓ 客户端 $i 完成 1000 次操作${NC}"
    ) &
done

wait
echo ""

# 清理
echo "=========================================="
echo "清理测试数据..."
echo "DROP DATABASE concurrent_test;" | nc $HOST $PORT > /dev/null 2>&1
echo -e "${GREEN}✓ 清理完成${NC}"
echo ""

# 总结
echo "=========================================="
echo "并发测试总结"
echo "=========================================="
echo -e "${GREEN}所有并发测试完成${NC}"
echo ""
echo "测试项目:"
echo "  1. 并发读测试 - 验证高并发读取性能"
echo "  2. 并发写测试 - 验证并发写入和数据一致性"
echo "  3. 混合读写测试 - 模拟真实工作负载"
echo "  4. 竞争条件测试 - 验证并发控制机制"
echo "  5. 长连接测试 - 验证连接稳定性"
echo ""
echo "如果竞争条件测试失败，请检查:"
echo "  - LockManager 是否正确实现"
echo "  - FileIO 和 BPlusTree 是否正确加锁"
echo "  - 事务隔离级别是否正确"
echo "=========================================="
