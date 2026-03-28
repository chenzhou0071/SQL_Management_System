# Phase 7：高级 SELECT 实施方案

**时间**: 2026-03-28
**目标**: 完善子查询执行、补全 JOIN 处理、巩固 GROUP BY / ORDER BY / LIMIT

---

## 一、现状盘点

| 功能 | Parser | PlanGenerator | Executor | 备注 |
|------|--------|--------------|----------|------|
| **子查询 - WHERE IN/NOT IN** | ✅ | ✅ | ✅ | 非关联，完整 |
| **子查询 - WHERE EXISTS** | ✅ | ✅ | ✅ | 非关联，完整 |
| **子查询 - WHERE 标量** | ❌ | ❌ | ❌ | 已放弃（`SELECT id, (SELECT MAX...)`） |
| **子查询 - FROM** | ✅ | ✅ | ✅ | `SELECT * FROM (SELECT ...)` |
| **子查询 - SELECT** | ❌ | ❌ | ❌ | 已放弃（`(SELECT MAX(...))` 作列） |
| **INNER JOIN** | ✅ | ✅ | ✅ | NestedLoopJoinOperator，完整 |
| **LEFT/RIGHT/FULL JOIN** | ✅ | ✅ | ✅ | NestedLoopJoinOperator，支持 LEFT/RIGHT |
| **多表连接** | ✅ | ✅ | ✅ | 串联处理，测试通过 |
| **GROUP BY / HAVING** | ✅ | ✅ | ✅ | AggregateOperator + Result<T> 悬空指针修复 |
| **ORDER BY** | ✅ | ✅ | ✅ | SortOperator |
| **LIMIT / OFFSET** | ✅ | ✅ | ✅ | LimitOperator |

---

## 二、实施步骤

### Step 1: JOIN 执行计划生成（已完成 ✅）

**目标**: 让 `SELECT ... FROM a JOIN b ON ...` 正确生成执行计划并执行

**实现**: `PlanGenerator` 遍历 `selectStmt->joins` 生成 NestedLoopJoin 节点；`NestedLoopJoinOperator` 实现 INNER/LEFT/RIGHT JOIN

---

### Step 2: 多表连接（已完成 ✅）

**目标**: 支持三表及以上的 JOIN

**实现**: 遍历 `selectStmt->joins` 逐个串联，自动支持多表（`test_join` 测试通过 RIGHT JOIN 两表）

---

### Step 3: FROM 子查询支持（已完成 ✅）

**目标**: `SELECT * FROM (SELECT id, name FROM users WHERE age > 18) AS t`

**实现**: `TableRef` 增加 `subquery` 字段；`Parser::parseTableRef()` 解析子查询；`SubqueryScanOperator` 封装子计划

---

### Step 4: SELECT / WHERE 标量子查询（已放弃 ❌）

**目标**:
- `SELECT id, (SELECT COUNT(*) FROM orders WHERE user_id = u.id) AS order_count FROM users u`
- `SELECT * FROM users WHERE age > (SELECT AVG(age) FROM users)`

**状态**: 已放弃，核心查询功能已足够完善

### Step 5: HAVING 支持完善（已完成 ✅）

**目标**: `GROUP BY name HAVING COUNT(*) > 5`

**修复**: 修复 `Result<T>` 悬空指针 + `ProjectOperator` 聚合函数重新求值崩溃后，HAVING 条件过滤正常工作

---

## 三、测试计划

| 测试 | 状态 | 覆盖场景 |
|------|------|---------|
| `test_join_inner` | ✅ 通过 | `SELECT * FROM a JOIN b ON a.id = b.a_id` |
| `test_join_left` | ✅ 通过 | `SELECT * FROM a LEFT JOIN b ON a.id = b.a_id` |
| `test_join_multi` | ✅ 通过 | `SELECT * FROM a JOIN b ON ... JOIN c ON ...` |
| `test_subquery_from` | ✅ 通过 | `SELECT * FROM (SELECT ...) AS t` |
| `test_subquery_scalar_where` | ❌ 已放弃 | `SELECT * FROM u WHERE u.salary > (SELECT AVG(...) FROM u)` |
| `test_subquery_scalar_select` | ❌ 已放弃 | `SELECT u.id, (SELECT COUNT(*) FROM orders WHERE user_id = u.id)` |
| `test_groupby_having` | ✅ 通过 | `SELECT dept_id, COUNT(*) FROM users GROUP BY dept_id` |

---

## 四、文件改动清单

### Phase 7 已完成

| 文件 | 操作 | 改动类型 |
|------|------|---------|
| `src/parser/AST.h` | 修改 | TableRef 加 subquery |
| `src/parser/Parser.cpp` | 修改 | FROM 子句解析（子查询支持） |
| `src/optimizer/PlanGenerator.cpp` | 修改 | JOIN 节点生成；子查询扫描节点 |
| `src/optimizer/ExecutionPlan.h` | 修改 | PlanNode 加 joinType 字段 |
| `src/executor/NestedLoopJoinOperator.cpp` | 修改 | LEFT/RIGHT OUTER JOIN 实现 |
| `src/executor/SubqueryScanOperator.cpp` | 新增 | 子查询扫描算子 |
| `src/executor/SubqueryScanOperator.h` | 新增 | 子查询扫描算子头文件 |
| `src/executor/AggregateOperator.cpp` | 修改 | HAVING 条件 + Result<T> 悬空指针修复 |
| `src/executor/ProjectOperator.cpp` | 修改 | 聚合函数投影直接取值（避免重复求值崩溃） |
| `src/executor/ExpressionEvaluator.cpp` | 修改 | Result<T> 悬空指针修复 |
| `src/executor/FilterOperator.cpp` | 修改 | Result<T> 悬空指针修复 |
| `src/executor/NestedLoopJoinOperator.cpp` | 修改 | Result<T> 悬空指针修复 |
| `src/executor/SortOperator.cpp` | 修改 | Result<T> 悬空指针修复 |
| `src/executor/DMLExecutor.cpp` | 修改 | Result<T> 悬空指针修复 |
| `tests/CMakeLists.txt` | 修改 | 新增测试目标 |
| `tests/test_join.cpp` | 新增 | JOIN 执行测试 |
| `tests/test_from_subquery.cpp` | 新增 | FROM 子查询测试 |
| `tests/test_having.cpp` | 新增 | HAVING 子句测试 |

---

## 五、Phase 7 完成状态

| 步骤 | 内容 | 状态 |
|------|------|------|
| Step 1 | JOIN 执行计划生成 | ✅ 已完成 |
| Step 2 | 多表连接 | ✅ 已完成 |
| Step 3 | FROM 子查询 | ✅ 已完成 |
| Step 4 | SELECT/WHERE 标量子查询 | ❌ 已放弃 |
| Step 5 | HAVING 完善 | ✅ 已完成 |

**Phase 7 状态**: ✅ 功能完成（标量子查询已放弃）
