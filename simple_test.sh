#!/bin/bash

# 统一的测试脚本

LOG_FILE="test_results.log"

echo "========================================" > $LOG_FILE
echo "  MiniSQL 服务器测试" >> $LOG_FILE
echo "========================================" >> $LOG_FILE
echo "" >> $LOG_FILE

# 1. 停止旧服务器
echo "步骤 1: 停止旧服务器..." >> $LOG_FILE
killall -9 minisql_server 2>/dev/null || true
sleep 1
echo "✓ 旧服务器已停止" >> $LOG_FILE
echo "" >> $LOG_FILE

# 2. 清理旧数据
echo "步骤 2: 清理旧数据..." >> $LOG_FILE
cd "/mnt/hgfs/share/SQL Management System/build_linux/bin"
rm -rf data/*_db
echo "✓ 数据已清理" >> $LOG_FILE
echo "" >> $LOG_FILE

# 3. 启动服务器
echo "步骤 3: 启动服务器..." >> $LOG_FILE
./minisql_server > server.log 2>&1 &
SERVER_PID=$!
echo "✓ 服务器已启动 (PID: $SERVER_PID)" >> $LOG_FILE
sleep 2

if ! ps -p $SERVER_PID > /dev/null; then
    echo "✗ 服务器启动失败" >> $LOG_FILE
    echo "查看日志:" >> $LOG_FILE
    tail -30 server.log >> $LOG_FILE
    cat $LOG_FILE
    exit 1
fi
echo "" >> $LOG_FILE

# 4. 运行测试
echo "步骤 4: 运行测试..." >> $LOG_FILE
cd "/mnt/hgfs/share/SQL Management System"
bash test_server_quick.sh >> test_output.log 2>&1

# 统计测试结果
PASS=$(grep -c "✓" test_output.log || echo "0")
FAIL=$(grep -c "✗" test_output.log || echo "0")

echo "" >> $LOG_FILE
echo "测试结果:" >> $LOG_FILE
echo "  通过: $PASS" >> $LOG_FILE
echo "  失败: $FAIL" >> $LOG_FILE
echo "" >> $LOG_FILE

if [ "$FAIL" -gt "0" ]; then
    echo "失败的测试:" >> $LOG_FILE
    grep -B1 "✗" test_output.log >> $LOG_FILE
fi

echo "========================================" >> $LOG_FILE
echo "" >> $LOG_FILE

# 显示结果
cat $LOG_FILE

echo ""
echo "详细日志文件:"
echo "   汇总: $LOG_FILE"
echo "   服务器: build_linux/bin/server.log"
echo "   测试: test_output.log"
echo ""
