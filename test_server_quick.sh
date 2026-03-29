#!/bin/bash

# MiniSQL 服务器快速功能测试

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
echo "1. 检查服务器..."
if ! nc -z $HOST $PORT 2>/dev/null; then
    echo -e "${RED}服务器未运行${NC}"
    exit 1
fi
echo -e "${GREEN}✓ 服务器运行正常${NC}"
echo ""

# DDL 测试
echo "2. DDL 测试..."
echo -n "  CREATE DATABASE... "
echo "CREATE DATABASE quick_test;" | nc -q 1 $HOST $PORT 2>/dev/null | grep -q "OK" && echo -e "${GREEN}✓${NC}" || echo -e "${RED}✗${NC}"

echo -n "  USE DATABASE... "
echo "USE quick_test;" | nc -q 1 $HOST $PORT 2>/dev/null | grep -q "OK" && echo -e "${GREEN}✓${NC}" || echo -e "${RED}✗${NC}"

echo -n "  CREATE TABLE... "
echo "CREATE TABLE test(id INT PRIMARY KEY, name VARCHAR(50));" | nc -q 1 $HOST $PORT 2>/dev/null | grep -q "OK" && echo -e "${GREEN}✓${NC}" || echo -e "${RED}✗${NC}"
echo ""

# INSERT 测试
echo "3. INSERT 测试..."
echo -n "  INSERT user1... "
echo "INSERT INTO test VALUES(1, 'Alice');" | nc -q 1 $HOST $PORT 2>/dev/null | grep -q "OK" && echo -e "${GREEN}✓${NC}" || echo -e "${RED}✗${NC}"

echo -n "  INSERT user2... "
echo "INSERT INTO test VALUES(2, 'Bob');" | nc -q 1 $HOST $PORT 2>/dev/null | grep -q "OK" && echo -e "${GREEN}✓${NC}" || echo -e "${RED}✗${NC}"

echo -n "  INSERT user3... "
echo "INSERT INTO test VALUES(3, 'Charlie');" | nc -q 1 $HOST $PORT 2>/dev/null | grep -q "OK" && echo -e "${GREEN}✓${NC}" || echo -e "${RED}✗${NC}"
echo ""

# 查询测试
echo "4. 查询测试..."
echo -n "  SELECT *... "
echo "SELECT * FROM test;" | nc -q 1 $HOST $PORT 2>/dev/null | grep -q "Alice" && echo -e "${GREEN}✓${NC}" || echo -e "${RED}✗${NC}"

echo -n "  UPDATE... "
echo "UPDATE test SET name='Alice Updated' WHERE id=1;" | nc -q 1 $HOST $PORT 2>/dev/null | grep -q "OK" && echo -e "${GREEN}✓${NC}" || echo -e "${RED}✗${NC}"

echo -n "  DELETE... "
echo "DELETE FROM test WHERE id=3;" | nc -q 1 $HOST $PORT 2>/dev/null | grep -q "OK" && echo -e "${GREEN}✓${NC}" || echo -e "${RED}✗${NC}"
echo ""

# 索引测试
echo "5. 索引测试..."
echo -n "  CREATE INDEX... "
echo "CREATE INDEX idx_name ON test(name);" | nc -q 1 $HOST $PORT 2>/dev/null | grep -q "OK" && echo -e "${GREEN}✓${NC}" || echo -e "${RED}✗${NC}"

echo -n "  DROP INDEX... "
echo "DROP INDEX idx_name ON test;" | nc -q 1 $HOST $PORT 2>/dev/null | grep -q "OK" && echo -e "${GREEN}✓${NC}" || echo -e "${RED}✗${NC}"
echo ""

# 错误处理测试
echo "6. 错误处理..."
echo -n "  重复数据库... "
echo "CREATE DATABASE quick_test;" | nc -q 1 $HOST $PORT 2>/dev/null | grep -q "exists" && echo -e "${GREEN}✓${NC}" || echo -e "${RED}✗${NC}"

echo -n "  不存在表... "
echo "SELECT * FROM nonexistent;" | nc -q 1 $HOST $PORT 2>/dev/null | grep -q "not found" && echo -e "${GREEN}✓${NC}" || echo -e "${RED}✗${NC}"
echo ""

echo "=========================================="
echo -e "${GREEN}测试完成！${NC}"
echo "=========================================="
