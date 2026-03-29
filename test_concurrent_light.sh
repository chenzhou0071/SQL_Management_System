#!/bin/bash

# MiniSQL 服务器轻量级并发测试

HOST="10.121.222.223"
PORT="3306"
NUM_CLIENTS=10
NUM_OPERATIONS=20

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "=========================================="
echo "  MiniSQL 轻量级并发测���"
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
(echo "CREATE DATABASE concurrent_test;"; echo "USE concurrent_test;"; echo "CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR(50), balance FLOAT);") | nc -w 3 $HOST $PORT > /dev/null 2>&1

# 初始化数据
echo "2. 初始化测试数据..."
for i in $(seq 1 20); do
    echo "INSERT INTO users VALUES($i, 'User$i', 1000.00);" | nc -w 3 $HOST $PORT > /dev/null 2>&1
done
echo -e "${GREEN}✓ 初始化完成${NC}"
echo ""

# 测试1: 并发读
echo "=========================================="
echo "测试1: 并发读测试"
echo "=========================================="

start_time=$(date +%s.%N)

for i in $(seq 1 $NUM_CLIENTS); do
    (
        for j in $(seq 1 $NUM_OPERATIONS); do
            echo "SELECT * FROM users WHERE id = $((RANDOM % 20 + 1));" | nc -q 1 $HOST $PORT > /dev/null 2>&1
        done
        echo -e "${GREEN}✓ 客户端 $i 完成${NC}"
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

start_time=$(date +%s.%N)

for i in $(seq 1 $NUM_CLIENTS); do
    (
        for j in $(seq 1 $NUM_OPERATIONS); do
            user_id=$((RANDOM % 20 + 1))
            new_balance=$((RANDOM % 10000))
            echo "UPDATE users SET balance = $new_balance WHERE id = $user_id;" | nc -q 1 $HOST $PORT > /dev/null 2>&1
        done
        echo -e "${GREEN}✓ 客户端 $i 完成${NC}"
    ) &
done

wait

end_time=$(date +%s.%N)
elapsed=$(echo "$end_time - $start_time" | bc)
ops_per_sec=$(echo "scale=2; $total_ops / $elapsed" | bc)

echo -e "${GREEN}✓ 并发写测试完成${NC}"
echo "总操作数: $total_ops"
echo "总耗时: ${elapsed}s"
echo "吞吐量: ${ops_per_sec} ops/sec"
echo ""

# 清理
echo "=========================================="
echo "清理测试数据..."
echo "DROP DATABASE concurrent_test;" | nc -q 1 $HOST $PORT > /dev/null 2>&1
echo -e "${GREEN}✓ 清理完成${NC}"
echo ""

echo "=========================================="
echo -e "${GREEN}轻量级并发测试完成${NC}"
echo "=========================================="
