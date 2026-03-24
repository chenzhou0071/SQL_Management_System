# MiniSQL 实现计划

## 实现顺序和模块划分

### 第一阶段：基础框架（1-2周）

#### 1. 项目搭建
- CMake 项目结构
- Qt 主窗口框架
- 日志系统

#### 2. 公共模块 (common/)
- **数据类型** - 定义所有支持的数据类型
- **值对象** - 统一的值表示
- **错误处理** - 错误码和异常��理

---

### 第二阶段：存储引擎（2-3周）

#### 3. 文件存储 (storage/)
- **FileIO** - 文件读写操作
- **TableManager** - 表结构管理
  - 创建表
  - 删除表
  - 获取表结构
  - 加载/保存表数据
- **数据持久化** - 内存数据到文件的转换

#### 4. B+树索引 (storage/)
- **B+树数据结构**
  - 节点定义（内部节点/叶子节点）
  - 插入操作
  - 删除操作
  - 查找操作
  - 范围查询
- **IndexManager** - 索引管理
  - 创建索引
  - 删除索引
  - 使用索引查询
- **索引文件管理** - 索引数据的持久化

---

### 第三阶段：SQL解析（3-4周）

#### 5. 词法分析器 (parser/)
- **Token 定义**
  - TokenType 枚举
  - Token 结构体
- **Lexer 实现**
  - 状态机实现
  - 关键字识别
  - 字符串识别
  - 数字识别
  - 注释跳过
- **测试** - 各种 SQL 语句的分词测试

#### 6. 语法分析器 (parser/)
- **AST 节点定义**
  - SelectStmt
  - InsertStmt
  - UpdateStmt
  - DeleteStmt
  - CreateStmt
  - DropStmt
  - 表达式节点
- **Parser 实现**
  - 递归下降分析
  - SELECT 解析
  - INSERT 解析
  - UPDATE 解析
  - DELETE 解析
  - CREATE 解析
  - DROP 解析
- **语法规则** - BNF 定义和实现

#### 7. 语义分析 (parser/)
- **表存在性检查**
- **字段存在性检查**
- **类型检查**
- **约束检查**

---

### 第四阶段：执行器（2-3周）

#### 8. 表达式计算 (executor/)
- **Expression 定义**
  - 算术表达式（+、-、*、/）
  - 逻辑表达式（AND、OR、NOT）
  - 比较表达式（=、>、<、>=、<=、!=）
  - 函数调用
- **表达式求值**
  - 常量折叠
  - 类型转换

#### 9. 数据扫描 (executor/)
- **TableScan** - 全表扫描
- **IndexScan** - 索引扫描
- **过滤条件** - WHERE 条件执行

#### 10. JOIN执行 (executor/)
- **NestedLoopJoin** - 嵌套循环连接
  - INNER JOIN
  - LEFT JOIN
- **连接条件** - ON 条件执行

#### 11. 聚合和排序 (executor/)
- **Aggregate** - 聚合函数
  - COUNT
  - SUM
  - AVG
  - MAX
  - MIN
- **GroupBy** - GROUP BY 执行
- **Sort** - ORDER BY 执行
- **Limit** - LIMIT 执行

---

### 第五阶段：优化器（1-2周）

#### 12. 规则优化器 (optimizer/)
- **RuleOptimizer**
  - 索引选择规则
  - 常量折叠
  - 消除冗余排序
- **索引选择** - 根据条件选择最优索引

#### 13. EXPLAIN (optimizer/)
- **执行计划生成**
  - 扫描类型（Table Scan/Index Scan）
  - 索引使用
  - 预估行数
- **执行计划展示** - 格式化输出

---

### 第六阶段：基础功能（2-3周）

#### 14. DDL执行 (executor/)
- **CreateDatabase**
- **CreateTable**
  - 解析字段定义
  - 解析约束
  - 创建表结构
- **DropDatabase**
- **DropTable**
- **AlterTable**
  - ADD COLUMN
  - DROP COLUMN
  - MODIFY COLUMN

#### 15. DML执行 (executor/)
- **Insert**
  - 单行插入
  - 批量插入
- **Update**
  - 单表更新
  - WHERE 条件
- **Delete**
  - 单表删除
  - WHERE 条件

#### 16. 基础查询 (executor/)
- **Select**
  - 简单查询
  - WHERE 条件
  - 列选择

---

### 第七阶段：高级查询（2-3周）

#### 17. 高级SELECT (executor/)
- **子查询**
  - WHERE 子查询
  - FROM 子查询
  - SELECT 子查询
- **JOIN**
  - INNER JOIN
  - LEFT JOIN
  - 多表连接
- **GROUP BY / HAVING**
- **ORDER BY / LIMIT**

---

### 第八阶段：事务（1-2周）

#### 18. 事务管理 (transaction/)
- **TransactionManager**
  - begin() - 开始事务
  - commit() - 提交事务
  - rollback() - 回滚事务
- **Operation** - 操作日志
  - execute() - 执行操作
  - undo() - 撤销操作
- **事务状态管理**
  - 自动提交模式
  - 手动提交模式

---

### 第九阶段：进阶功能（2-3周）

#### 19. 视图 (storage/)
- **ViewManager**
  - 创建视图
  - 删除视图
  - 查询视图（替换为定义的 SELECT）

#### 20. 存储过程 (parser/ & executor/)
- **存储过程解析**
  - 参数解析（IN/OUT/INOUT）
  - 变量声明
  - 流程控制（IF/WHILE）
- **ProcedureManager**
  - 创建存储过程
  - 删除存储过程
  - 调用存储过程

#### 21. 触发器 (parser/ & executor/)
- **触发器解析**
  - 触发时机（BEFORE/AFTER）
  - 触发事件（INSERT/UPDATE/DELETE）
  - 触发逻辑
- **TriggerManager**
  - 创建触发器
  - 删除触发器
  - 执行触发器

---

### 第十阶段：GUI界面（2-3周）

#### 22. 主界面 (application/)
- **MainWindow**
  - 布局设计（三栏式）
  - 数据库树形结构
  - 表格展示
  - 终端区
- **主题切换** - 亮色/暗色主题

#### 23. 终端区 (application/)
- **SQL 输入**
  - 多行输入
  - 语法高亮
  - 历史记录
- **结果展示**
  - 表格展示
  - 执行计划展示
  - 错误信息展示

#### 24. 弹窗对话框 (application/)
- **新建库对话框**
- **新建表对话框**
  - 字段定义
  - 类型选择
  - 约束设置
- **插入数据对话框**
- **修改数据对话框**
- **删除数据对话框**
- **查询对话框**
  - 字段选择
  - WHERE 条件
  - ORDER BY
  - LIMIT
- **EXPLAIN 对话框**
  - SQL 输入
  - 执行计划展示

---

### 第十一阶段：测试和优化（1-2周）

#### 25. 测试
- **单元测试**
  - 词法分析测试
  - 语法分析测试
  - B+树测试
  - 索引测试
- **集成测试**
  - DDL 测试
  - DML 测试
  - 查询测试
  - 事务测试

#### 26. 优化
- **性能优化**
  - 查询性能
  - 索引性能
- **Bug 修复**
- **代码重构**

---

## 项目目录结构

```
MiniSQL/
├── src/
│   ├── main.cpp                    # 入口文件
│   ├── application/                # 应用层
│   │   ├── MainWindow.h/cpp        # GUI主窗口
│   │   ├── CLIMode.h/cpp           # 终端模式
│   │   ├── Dialogs/                # 对话框
│   │   │   ├── CreateDbDialog.h/cpp
│   │   │   ├── CreateTableDialog.h/cpp
│   │   │   ├── InsertDialog.h/cpp
│   │   │   ├── UpdateDialog.h/cpp
│   │   │   ├── DeleteDialog.h/cpp
│   │   │   ├── QueryDialog.h/cpp
│   │   │   └── ExplainDialog.h/cpp
│   │   └── ...
│   ├── parser/                     # 解析层
│   │   ├── Lexer.h/cpp             # 词法分析器
│   │   ├── Parser.h/cpp            # 语法分析器
│   │   ├── AST.h                   # 抽象语法树定义
│   │   ├── Token.h                 # Token定义
│   │   └── ...
│   ├── optimizer/                  # 优化器
│   │   ├── Optimizer.h/cpp         # 优化器主类
│   │   ├── RuleOptimizer.h/cpp     # 规则优化
│   │   ├── IndexSelector.h/cpp     # 索引选择
│   │   ├── Explain.h/cpp           # EXPLAIN执行计划
│   │   └── ...
│   ├── executor/                   # 执行器
│   │   ├── Executor.h/cpp          # 执行器主类
│   │   ├── Expression.h/cpp        # 表达式计算
│   │   ├── TableScan.h/cpp         # 表扫描
│   │   ├── IndexScan.h/cpp         # 索引扫描
│   │   ├── JoinExecutor.h/cpp      # JOIN执行
│   │   ├── Aggregate.h/cpp         # 聚合函数
│   │   ├── Sort.h/cpp              # 排序
│   │   ├── DDLExecutor.h/cpp       # DDL执行
│   │   ├── DMLExecutor.h/cpp       # DML执行
│   │   └── ...
│   ├── storage/                    # 存储引擎
│   │   ├── TableManager.h/cpp      # 表管理
│   │   ├── IndexManager.h/cpp      # 索引管理
│   │   ├── BPlusTree.h/cpp         # B+树实现
│   │   ├── FileIO.h/cpp            # 文件读写
│   │   ├── ViewManager.h/cpp       # 视图管理
│   │   ├── ProcedureManager.h/cpp  # 存储过程管理
│   │   ├── TriggerManager.h/cpp    # 触发器管理
│   │   └── ...
│   ├── transaction/                # 事务管理
│   │   ├── TransactionManager.h/cpp # 事务管理器
│   │   ├── Operation.h/cpp          # 操作日志
│   │   └── ...
│   ├── common/                     # 公共组件
│   │   ├── Type.h/cpp              # 数据类型
│   │   ├── Value.h/cpp             # 值对象
│   │   ├── Error.h/cpp             # 错误处理
│   │   ├── Logger.h/cpp            # 日志系统
│   │   └── ...
│   └── CMakeLists.txt
├── data/                           # 数据目录（运行时生成）
├── tests/                          # 测试代码
│   ├── test_lexer.cpp
│   ├── test_parser.cpp
│   ├── test_btree.cpp
│   └── ...
├── docs/
│   └── plans/                      # 设计文档
└── README.md
```

---

## 总计：约 4-6 个月

**优先级**：按阶段顺序实现，每完成一个阶段可以测试验证功能。

**建议**：
1. 先实现核心功能（存储+解析+执行）
2. 再做优化器和进阶功能
3. 最后做 GUI

**里程碑**：
- 第 6 周：完成存储引擎，可以创建表和插入数据
- 第 12 周：完成 SQL 解析和执行，可以执行简单查询
- 第 18 周：完成优化器和高级查询
- 第 22 周：完成事务和进阶功能
- 第 26 周：完成 GUI 和测试
