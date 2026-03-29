#!/bin/bash

# MiniSQL 服务器快速功能测试
# 用于快速��证服务器基本功能

HOST="10.121.222.223"
PORT="3306"

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo "=========================================="
echo "  MiniSQL 服务器快速测试"
echo "=========================================="
echo ""

# 检查服务器
echo "1. 检查服务器状态..."
if ! nc -z $HOST $PORT 2>/dev/null; then
    echo -e "${RED}服务器未运行${NC}"
    exit 1
fi
echo -e "${GREEN}✓ 服务器运行正常${NC}"
echo ""

# 创建数据库和表
echo "2. 创建数据库和表..."
echo "CREATE DATABASE quick_test;" | nc -q 1 $HOST $PORT | grep -q "OK" && echo -e "${GREEN}✓ 创建数据库成功${NC}" || echo -e "${RED}✗ 失败${NC}"
echo "USE quick_test;" | nc -q 1 $HOST $PORT | grep -q "quick_test" && echo -e "${GREEN}✓ 使用数据库成功${NC}" || echo -e "${RED}✗ 失败${NC}"
echo "CREATE TABLE test(id INT PRIMARY KEY, name VARCHAR(50));" | nc -q 1 $HOST $PORT | grep -q "OK" && echo -e "${GREEN}✓ 创建表成功${NC}" || echo -e "${RED}✗ 失败${NC}"
echo ""

# 插入数据
echo "3. 插入数据..."
echo "INSERT INTO test VALUES(1, 'Alice');" | nc -q 1 $HOST $PORT | grep -q "OK" && echo -e "${GREEN}✓ 插入数据成功${NC}" || echo -e "${RED}✗ 失败${NC}"
echo "INSERT INTO test VALUES(2, 'Bob');" | nc -q 1 $HOST $PORT | grep -q "OK" && echo -e "${GREEN}✓ 插入数据成功${NC}" || echo -e "${RED}✗ 失败${NC}"
echo "INSERT INTO test VALUES(3, 'Charlie');" | nc -q 1 $HOST $PORT | grep -q "OK" && echo -e "${GREEN}✓ 插入数据成功${NC}" || echo -e "${RED}✗ 失败${NC}"
echo ""

# 查询数据
echo "4. 查询数据..."
echo "SELECT * FROM test;" | nc -q 1 $HOST $PORT | grep -q "Alice" && echo -e "${GREEN}✓ 查询数据成功${NC}" || echo -e "${RED}✗ 失败${NC}"
echo ""

# 更新数据
echo "5. 更新数据..."
echo "UPDATE test SET name = 'Alice Updated' WHERE id = 1;" | nc -q 1 $HOST $PORT | grep -q "OK" && echo -e "${GREEN}✓ 更新数据成功${NC}" || echo -e "${RED}✗ 失败${NC}"
echo ""

# 删除数据
echo "6. 删除数据..."
echo "DELETE FROM test WHERE id = 3;" | nc -q 1 $HOST $PORT | grep -q "OK" && echo -e "${GREEN}✓ 删除数据成功${NC}" || echo -e "${RED}✗ 失败${NC}"
echo ""

# 创建索引
echo "7. 创建索引..."
echo "CREATE INDEX idx_name ON test(name);" | nc -q 1 $HOST $PORT | grep -q "OK" && echo -e "${GREEN}✓ 创建索引成功${NC}" || echo -e "${RED}✗ 失败${NC}"
echo ""

# 聚合查询
echo "8. 聚合查询..."
echo "SELECT COUNT(*) FROM test;" | nc -q 1 $HOST $PORT | grep -q "OK" && echo -e "${GREEN}✓ 聚合查询成功${NC}" || echo -e "${RED}✗ 失败${NC}"
echo ""

# 保留测试数据
echo "9. 测试完成..."
echo -e "${GREEN}✓ 测试数据已保留${NC}"
echo ""

echo "=========================================="
echo -e "${GREEN}快速测试完成！${NC}"
echo "=========================================="
