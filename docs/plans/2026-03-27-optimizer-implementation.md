# Phase 5: 查询优化器实施方案

**实现时间**: 2026-03-27 ~ 预计 1-2 周
**目标**: 实现完整的查询优化器，包括查询重写、规则优化、代价优化和执行计划生成

---

## 一、架构设计

### 1.1 整体架构

```
┌─────────────────────────────────────────���───────────────┐
│                    Executor (现有)                      │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│              Query Optimizer (新增)                     │
├─────────────────────────────────────────────────────────┤
│  ┌─────────────┐    ┌──────────────┐    ┌────────────┐ │
│  │ Query       │───▶│ Rule-Based   │───▶│ Cost-Based│ │
│  │ Rewriter    │    │ Optimizer    │    │ Optimizer  │ │
│  │ (查询重写)   │    │ (RBO)        │    │ (CBO)      │ │
│  └─────────────┘    └──────────────┘    └────────────┘ │
│                                                     │    │
│                                                     ▼    ▼
│                                      ┌────────────────────┐
│                                      │ Plan Generator     │
│                                      │ (执行计划生成)      │
│                                      └────────────────────┘
└─────────────────────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│              Physical Operators (现有)                  │
└─────────────────────────────────────────────────────────┘
```

### 1.2 核心组件

```
src/optimizer/
├── QueryRewriter.h/cpp         # 查询重写器
├── RuleOptimizer.h/cpp         # 规则优化器 (RBO)
├── CostOptimizer.h/cpp         # 代价优化器 (CBO)
├── PlanGenerator.h/cpp         # 执行计划生成器
├── IndexSelector.h/cpp         # 索引选择器
├── Statistics.h/cpp            # 统计信息管理
├── ExecutionPlan.h             # 执行计划定义
└── ExplainHandler.h/cpp        # EXPLAIN 处理器
```

---

## 二、详细设计

### 2.1 查询重写器 (QueryRewriter)

**功能**: 在不改变语义的前提下优化查询语句

**优化规则**:

1. **常量折叠**
   ```sql
   -- 原始查询
   SELECT * FROM users WHERE age > 18 + 5 AND status = 1

   -- 重写后
   SELECT * FROM users WHERE age > 23 AND status = 1
   ```

2. **消除冗余条件**
   ```sql
   -- 原始查询
   SELECT * FROM users WHERE age > 18 OR age = 20

   -- 重写后
   SELECT * FROM users WHERE age > 18
   ```

3. **谓词下推**
   ```sql
   -- 原始查询
   SELECT * FROM (SELECT * FROM users) WHERE age > 18

   -- 重写后（尽早过滤）
   SELECT * FROM users WHERE age > 18
   ```

4. **子查询展开**（Phase 6 实现，预留接口）

**数据结构**:
```cpp
class QueryRewriter {
public:
    // 重写 SELECT 语句
    static Result<parser::SelectStmt*> rewriteSelect(parser::SelectStmt* stmt);

private:
    // 常量折叠
    static Result<parser::ExprPtr> foldConstants(parser::ExprPtr expr);

    // 消除冗余条件
    static Result<parser::ExprPtr> eliminateRedundantConditions(parser::ExprPtr expr);

    // 谓词下推
    static Result<void> pushDownPredicates(parser::SelectStmt* stmt);
};
```

---

### 2.2 规则优化器 (RuleOptimizer)

**功能**: 基于启发式规则优化执行计划

**优化规则（按优先级）**:

**优先级 1: 索引使用规则**
- ✅ 主键/唯一索引优先
- ✅ 复合索引最左前缀匹配
- ✅ 等值查询优先于范围查询
- ✅ 避免 SELECT *（只查询需要的列）

**优先级 2: 连接优化**
- ✅ 小表驱动大表（Nested Loop Join）
- ✅ 内连接优先于外连接
- ✅ 连接列上有索引优先

**优先级 3: 排序优化**
- ✅ 消除冗余排序（已有索引排序）
- ✅ 利用索引排序（避免 filesort）

**优先级 4: 聚合优化**
- ✅ 聚合前过滤（HAVING → WHERE）
- ✅ 利用覆盖索引（避免回表）

**数据结构**:
```cpp
struct OptimizerRule {
    std::string name;
    int priority;
    std::function<Result<void>(OperatorPtr)> apply;
};

class RuleOptimizer {
public:
    // 应用所有优化规则
    static Result<OperatorPtr> optimize(OperatorPtr plan);

private:
    static std::vector<OptimizerRule> rules_;

    // 索引使用规则
    static Result<void> applyIndexRules(OperatorPtr plan);

    // 连接顺序优化
    static Result<void> applyJoinOrderRules(OperatorPtr plan);

    // 排序优化
    static Result<void> applySortRules(OperatorPtr plan);
};
```

---

### 2.3 代价优化器 (CostOptimizer)

**功能**: 基于代价模型选择最优执行计划

**代价模型**:
```
Total Cost = CPU Cost + IO Cost

CPU Cost:
- TableScan: rows * 0.01
- IndexScan: rows * 0.005
- NestedLoopJoin: outer_rows * inner_rows * 0.02
- Filter: rows * 0.01
- Sort: rows * log(rows) * 0.05

IO Cost:
- TableScan: table_pages
- IndexScan: index_pages + result_rows * 0.1
- NestedLoopJoin: outer_rows * inner_pages
```

**Selectivity 估算**:
```cpp
double estimateSelectivity(parser::ExprPtr condition) {
    // 等值条件: 1 / distinct_values
    // 范围条件: (max - min) / (range_max - range_min)
    // AND: selectivity_left * selectivity_right
    // OR: selectivity_left + selectivity_right - product
}
```

**数据结构**:
```cpp
struct CostEstimate {
    double cpuCost;      // CPU 代价
    double ioCost;       // IO 代价
    double totalCost;    // 总代价
    int estimatedRows;   // 预估行数
};

class CostOptimizer {
public:
    // 优化执行计划（枚举并选择最优）
    static Result<OperatorPtr> optimize(OperatorPtr plan);

    // 估算子树代价
    static Result<CostEstimate> estimateCost(OperatorPtr node);

private:
    // 估算扫描代价
    static Result<CostEstimate> estimateScanCost(
        const std::string& tableName,
        parser::ExprPtr filter);

    // 估算连接代价
    static Result<CostEstimate> estimateJoinCost(
        OperatorPtr left,
        OperatorPtr right,
        parser::ExprPtr condition);
};
```

---

### 2.4 索引选择器 (IndexSelector)

**功能**: 为查询条件选择最优索引

**选择算法**:
```
1. 收集表上所有可用索引
2. 提取 WHERE 子句中的所有条件
3. 匹配条件与索引：
   - 等值条件优先
   - 复合索引最左前缀
   - 范围条件次之
4. 计算每个索引的 selectivity
5. 选择 selectivity 最高的索引
```

**数据结构**:
```cpp
struct IndexMatch {
    std::string indexName;
    double selectivity;      // 选择性（0-1，越小越好）
    int matchedColumns;      // 匹配的列数
    bool hasEquality;        // 是否有等值条件
    IndexDef indexDef;
};

class IndexSelector {
public:
    // 为查询选择最优索引
    static Result<IndexMatch> selectIndex(
        const std::string& tableName,
        parser::ExprPtr whereClause);

private:
    // 提取条件中的列引用
    static std::vector<std::string> extractColumns(parser::ExprPtr expr);

    // 匹配索引
    static Result<IndexMatch> matchIndex(
        const IndexDef& index,
        const std::vector<std::string>& columns,
        const std::vector<parser::ExprPtr>& conditions);

    // 计算 selectivity
    static double calculateSelectivity(
        const IndexMatch& match,
        const TableStatistics& stats);
};
```

---

### 2.5 统计信息管理 (Statistics)

**功能**: 收集和维护表的统计信息

**统计内容**:
```json
{
  "tableName": "users",
  "rowCount": 1000,
  "tableSize": 102400,
  "lastAnalyzed": "2026-03-27 10:00:00",
  "columns": [
    {
      "columnName": "id",
      "distinctValues": 1000,
      "nullCount": 0,
      "avgLength": 4,
      "minValue": 1,
      "maxValue": 1000,
      "histogram": [...]  // 直方图（可选）
    },
    {
      "columnName": "age",
      "distinctValues": 50,
      "nullCount": 10,
      "avgLength": 4,
      "minValue": 18,
      "maxValue": 70
    }
  ]
}
```

**数据结构**:
```cpp
struct ColumnStatistics {
    std::string columnName;
    int64_t distinctValues;
    int64_t nullCount;
    double avgLength;
    Value minValue;
    Value maxValue;
    std::vector<int64_t> histogram;  // 等宽直方图
};

struct TableStatistics {
    std::string tableName;
    int64_t rowCount;
    int64_t tableSize;
    std::string lastAnalyzed;
    std::vector<ColumnStatistics> columns;
};

class Statistics {
public:
    // 分析表并收集统计信息
    static Result<void> analyzeTable(const std::string& dbName, const std::string& tableName);

    // 获取表统计信息
    static Result<TableStatistics> getTableStatistics(const std::string& dbName, const std::string& tableName);

    // 加载/保存统计信息
    static Result<void> loadStatistics(const std::string& dbName, const std::string& tableName);
    static Result<void> saveStatistics(const std::string& dbName, const std::string& tableName);

private:
    static std::string getStatsFilePath(const std::string& dbName, const std::string& tableName);
};
```

**文件格式**: `data/dbname/table.stats`

---

### 2.6 执行计划生成器 (PlanGenerator)

**功能**: 生成优化的物理执行计划

**执行计划节点**:
```cpp
enum class ScanType {
    FULL_SCAN,      // 全表扫描
    INDEX_SCAN,     // 索引扫描
    COVERING_SCAN   // 覆盖索引扫描（不需要回表）
};

struct PlanNode {
    std::string nodeId;
    std::string operatorType;    // "TableScan", "IndexScan", "NestedLoopJoin", etc.
    ScanType scanType;
    std::string tableName;
    std::string indexName;       // 使用的索引
    std::vector<std::string> columns;  // 涉及的列
    parser::ExprPtr condition;   // 过滤条件
    double cost;                 // 预估代价
    int estimatedRows;           // 预估行数
    std::vector<std::shared_ptr<PlanNode>> children;  // 子节点
};
```

**数据结构**:
```cpp
class PlanGenerator {
public:
    // 生成执行计划
    static Result<std::shared_ptr<PlanNode>> generate(parser::SelectStmt* stmt);

    // 将逻辑计划转换为物理算子
    static Result<OperatorPtr> materialize(std::shared_ptr<PlanNode> plan);

private:
    // 生成扫描节点
    static Result<std::shared_ptr<PlanNode>> generateScanNode(
        const std::string& tableName,
        parser::ExprPtr whereClause);

    // 生成连接节点
    static Result<std::shared_ptr<PlanNode>> generateJoinNode(
        std::shared_ptr<PlanNode> left,
        std::shared_ptr<PlanNode> right,
        parser::ExprPtr condition);
};
```

---

### 2.7 EXPLAIN 处理器 (ExplainHandler)

**功能**: 解析和展示执行计划

**输出格式**:
```
+----+-------------+-------+--------+---------------+---------+------+-------+
| id | select_type | table | type   | possible_keys | key     | rows | Extra |
+----+-------------+-------+--------+---------------+---------+------+-------+
|  1 | SIMPLE      | users | range  | idx_age       | idx_age |  100  |       |
|  2 | SIMPLE      | orders| eq_ref | PRIMARY       | PRIMARY |    1  |       |
+----+-------------+-------+--------+---------------+---------+------+-------+
```

**字段说明**:
- `id`: 查询序列号
- `select_type`: SIMPLE, PRIMARY, SUBQUERY, DERIVED, UNION
- `table`: 表名
- `type`: 访问类型（ALL, index, range, ref, eq_ref, const）
- `possible_keys`: 可能使用的索引
- `key`: 实际使用的索引
- `rows`: 预估扫描行数
- `Extra`: 额外信息（Using index, Using filesort, Using temporary）

**数据结构**:
```cpp
struct ExplainRow {
    int id;
    std::string selectType;     // SIMPLE, PRIMARY, SUBQUERY, etc.
    std::string tableName;
    std::string accessType;     // ALL, index, range, ref, eq_ref, const
    std::string possibleKeys;
    std::string key;
    int rows;
    std::vector<std::string> extra;
};

class ExplainHandler {
public:
    // 生成执行计划
    static Result<std::vector<ExplainRow>> explain(parser::SelectStmt* stmt);

    // 格式化输出
    static std::string formatExplain(const std::vector<ExplainRow>& rows);

    // JSON 格式输出
    static std::string formatExplainJSON(const std::vector<ExplainRow>& rows);
};
```

**访问类型映射**:
```cpp
std::string getAccessType(ScanType scanType, const IndexMatch& match) {
    if (scanType == FULL_SCAN) return "ALL";
    if (match.hasEquality && match.selectivity < 0.01) return "const";
    if (match.hasEquality) return "eq_ref";
    if (match.selectivity < 0.1) return "ref";
    return "range";
}
```

---

## 三、实施计划

### 3.1 任务分解

#### Task 1: 统计信息管理 (2天)
- [ ] 实现 `Statistics` 类
- [ ] 实现 `ANALYZE TABLE` 语句解析
- [ ] 实现统计信息收集（行数、distinct 值、min/max）
- [ ] 实现统计信息持久化（`.stats` 文件）
- [ ] 编写测试

#### Task 2: 索引选择器 (2天)
- [ ] 实现 `IndexSelector` 类
- [ ] 实现条件列提取
- [ ] 实现索引匹配算法
- [ ] 实现 selectivity 计算
- [ ] 编写测试

#### Task 3: 查询重写器 (2天)
- [ ] 实现 `QueryRewriter` 类
- [ ] 实现常量折叠
- [ ] 实现冗余条件消除
- [ ] 实现谓词下推
- [ ] 编写测试

#### Task 4: 规则优化器 (2天)
- [ ] 实现 `RuleOptimizer` 类
- [ ] 实现索引使用规则
- [ ] 实现连接顺序优化
- [ ] 实现排序优化
- [ ] 编写测试

#### Task 5: 代价优化器 (3天)
- [ ] 实现 `CostEstimate` 结构
- [ ] 实现代价估算函数
- [ ] 实现 `CostOptimizer` 类
- [ ] 实现多计划枚举与选择
- [ ] 编写测试

#### Task 6: 执行计划生成器 (2天)
- [ ] 实现 `ExecutionPlan` 定义
- [ ] 实现 `PlanGenerator` 类
- [ ] 实现逻辑计划到物理算子转换
- [ ] 集成到现有 `Executor`
- [ ] 编写测试

#### Task 7: EXPLAIN 处理器 (2天)
- [ ] 实现 `EXPLAIN` 语句解析
- [ ] 实现 `ExplainHandler` 类
- [ ] 实现表格格式输出
- [ ] 实现 JSON 格式输出
- [ ] 编写测试

#### Task 8: 集成与测试 (2天)
- [ ] 集成所有优化器组件
- [ ] 更新 `Executor::buildExecutionPlan()`
- [ ] 端到端测试
- [ ] 性能测试
- [ ] 文档更新

---

### 3.2 依赖关系

```
Task 1 (Statistics) ──┐
                      ├──▶ Task 2 (IndexSelector) ──┐
Task 3 (QueryRewriter)┤                            │
                      ├──▶ Task 4 (RuleOptimizer) ──┼──▶ Task 6 (PlanGenerator)
                                                   │
Task 5 (CostOptimizer) ────────────────────────────┘
                                                      │
Task 7 (ExplainHandler) ─────────────────────────────┤
                                                     │
                                                     ▼
                                              Task 8 (Integration)
```

---

## 四、测试用例

### 4.1 索引选择测试

```sql
-- 测试用例 1: 等值查询选择主键
EXPLAIN SELECT * FROM users WHERE id = 100;
-- 预期: type=const, key=PRIMARY

-- 测试用例 2: 范围查询���择普通索引
EXPLAIN SELECT * FROM users WHERE age > 18;
-- 预期: type=range, key=idx_age

-- 测试用例 3: 复合索引最左前缀
CREATE INDEX idx_name_age ON users(name, age);
EXPLAIN SELECT * FROM users WHERE name = 'Alice' AND age > 20;
-- 预期: key=idx_name_age, type=range

-- 测试用例 4: 无索引全表扫描
EXPLAIN SELECT * FROM users WHERE status = 1;
-- 预期: type=ALL, key=NULL
```

### 4.2 查询重写测试

```sql
-- 测试用例 1: 常量折叠
SELECT * FROM users WHERE age > 10 + 5;
-- 重写后: WHERE age > 15

-- 测试用例 2: 冗余条件消除
SELECT * FROM users WHERE age > 18 OR age = 20;
-- 重写后: WHERE age > 18

-- 测试用例 3: 谓词下推
SELECT * FROM (SELECT * FROM users) WHERE age > 18;
-- 重写后: SELECT * FROM users WHERE age > 18
```

### 4.3 连接优化测试

```sql
-- 测试用例 1: 小表驱动大表
EXPLAIN SELECT * FROM orders JOIN users ON orders.user_id = users.id;
-- 预期: users 作为外表（小表）

-- 测试用例 2: 索引连接
CREATE INDEX idx_user_id ON orders(user_id);
EXPLAIN SELECT * FROM users JOIN orders ON users.id = orders.user_id;
-- 预期: orders 使用 idx_user_id
```

### 4.4 排序优化测试

```sql
-- 测试用例 1: 利用索引排序
CREATE INDEX idx_age ON users(age);
EXPLAIN SELECT * FROM users WHERE age > 18 ORDER BY age;
-- 预期: Extra=Using index

-- 测试用例 2: 冗余排序消除
EXPLAIN SELECT * FROM users ORDER BY age;
-- 如果有 idx_age，不应出现 filesort
```

---

## 五、性能基准

### 5.1 测试数据

```sql
-- 创建测试表
CREATE TABLE users (
    id INT PRIMARY KEY,
    name VARCHAR(50),
    age INT,
    email VARCHAR(100),
    status INT
);

-- 插入 10,000 行数据
INSERT INTO users VALUES (1, 'Alice', 25, 'alice@example.com', 1);
-- ... 更多数据

-- 创建索引
CREATE INDEX idx_age ON users(age);
CREATE INDEX idx_status ON users(status);
CREATE INDEX idx_name_age ON users(name, age);
```

### 5.2 性能指标

| 查询类型 | 无优化 | 优化后 | 提升 |
|---------|--------|--------|------|
| 主键查询 | 10ms   | 1ms    | 10x  |
| 索引范围查询 | 100ms | 10ms   | 10x  |
| 全表扫描 | 500ms  | 500ms  | 1x   |
| 两表连接 | 1000ms | 200ms  | 5x   |
| 排序 | 300ms  | 50ms   | 6x   |

---

## 六、文件清单

### 6.1 新增文件

```
src/optimizer/
├── QueryRewriter.h/cpp         # 查询重写器
├── RuleOptimizer.h/cpp         # 规则优化器
├── CostOptimizer.h/cpp         # 代价优化器
├── PlanGenerator.h/cpp         # 执行计划生成器
├── IndexSelector.h/cpp         # 索引选择器
├── Statistics.h/cpp            # 统计信息管理
├── ExecutionPlan.h             # 执行计划定义
└── ExplainHandler.h/cpp        # EXPLAIN 处理器

tests/
├── test_query_rewriter.cpp     # 查询重写测试
├── test_index_selector.cpp     # 索引选择测试
├── test_rule_optimizer.cpp     # 规则优化测试
├── test_cost_optimizer.cpp     # 代价优化测试
├── test_plan_generator.cpp     # 计划生成测试
├── test_explain.cpp            # EXPLAIN 测试
└── test_statistics.cpp         # 统计信息测试
```

### 6.2 修改文件

```
src/parser/
├── AST.h                       # 添加 ExplainStmt
├── Parser.h/cpp                # 解析 EXPLAIN 语句

src/executor/
├── Executor.h/cpp              # 集成优化器
└── CMakeLists.txt              # 添加优化器模块

src/storage/
├── TableManager.h/cpp          # 添加 ANALYZE 支持

data/
└── [dbname]/[table].stats      # 统计信息文件
```

---

## 七、里程碑

| 里程碑 | 日期 | 目标 |
|--------|------|------|
| M1: 统计信息 | Day 2 | 完成 Statistics + IndexSelector |
| M2: 查询重写 | Day 4 | 完成 QueryRewriter |
| M3: 规则优化 | Day 6 | 完成 RuleOptimizer |
| M4: 代价优化 | Day 9 | 完成 CostOptimizer |
| M5: 计划生成 | Day 11 | 完成 PlanGenerator + 集成 |
| M6: EXPLAIN | Day 13 | 完成 ExplainHandler |
| M7: 测试与文档 | Day 15 | 全部测试通过 + 文档更新 |

---

## 八、风险与挑战

### 8.1 技术风险

| 风险 | 影响 | 缓解措施 |
|------|------|----------|
| 统计信息不准确 | 优化效果差 | 定期 ANALYZE，自适应调整 |
| 代价估算偏差大 | 选择错误计划 | 简化模型，RBO 作为兜底 |
| 优化器本身开销大 | 查询变慢 | 缓存执行计划，复杂查询才优化 |
| 多表连接组合爆炸 | 优化时间长 | 限制枚举空间，动态规划 |

### 8.2 实现建议

1. **渐进式实现**: 先实现 RBO，再逐步添加 CBO
2. **可观测性**: 添加详细的日志，记录优化过程
3. **可配置性**: 允许用户关闭优化器（`SET optimizer_switch='off'`）
4. **测试驱动**: 每个组件都先写测试，再实现功能

---

## 九、后续扩展

### 9.1 Phase 6 预留功能

- **子查询优化**: 相关子查询、IN 子查询扁平化
- **物化视图**: 自动查询重写
- **并行执行**: 并行扫描、并行连接
- **自适应优化**: 根据执行反馈调整计划

### 9.2 高级优化

- **分区裁剪**: 分区表自动跳过无关分区
- **连接算法选择**: Hash Join, Sort-Merge Join
- **批量处理**: Batch Nested Loop Join
- **列式存储**: 列存优化

---

## 十、参考资料

1. **MySQL 8.0 Optimizer**: https://dev.mysql.com/doc/refman/8.0/en/optimizer.html
2. **PostgreSQL Query Planning**: https://www.postgresql.org/docs/current/query-plan.html
3. **The Art of SQL**: 书籍
4. **Database System Concepts**: Chapter 15 - Query Optimization

---

*文档创建时间: 2026-03-27*
*预计完成时间: 2026-04-10*
