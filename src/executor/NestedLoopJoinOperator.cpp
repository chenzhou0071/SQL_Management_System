#include "NestedLoopJoinOperator.h"
#include "../common/Logger.h"

namespace minisql {
namespace executor {

// ============================================================
// 嵌套循环连接算子 - 实现INNER/LEFT/RIGHT JOIN
// ============================================================
// 算法原理：
// 基本的嵌套循环连接：
//   for each row in left_table:
//       for each row in right_table:
//           if join_condition matches:
//               output joined row
//
// 扩展支持LEFT/RIGHT JOIN：
// - LEFT JOIN: 如果左行未匹配任何右行，输出左行+NULL右行
// - RIGHT JOIN: 两阶段处理
//   阶段1：正常嵌套循环，标记匹配的右行
//   阶段2：输出未匹配的右行（NULL左行+右行）
//
// 实现策略：
// 1. 将右表物化到内存（rightRows_），避免重复扫描
// 2. 左表逐行扫描，与右表所有行尝试连接
// 3. 使用RowContext支持带表名前缀的列名解析（如users.id）
//
// 时间复杂度：O(左表行数 × 右表行数)
// 空间复杂度：O(右表行数) - 右表物化到内存
// ============================================================

// ============================================================
// 构造函数
// ============================================================
// 参数：
//   left      - 左表算子
//   right     - 右表算子
//   joinCond  - 连接条件（如 u.id = o.user_id）
//   joinType  - 连接类型（INNER/LEFT/RIGHT）
// ============================================================
NestedLoopJoinOperator::NestedLoopJoinOperator(OperatorPtr left, OperatorPtr right,
                                              parser::ExprPtr joinCond, JoinType joinType)
    : left_(left), right_(right), joinCond_(joinCond), joinType_(joinType),
      isOpen_(false), rightRowIndex_(0), leftRowMatched_(false),
      rightRowsIndex_(0), inRightJoinPhase2_(false) {
    children_.push_back(left);
    children_.push_back(right);
}

NestedLoopJoinOperator::~NestedLoopJoinOperator() {
    if (isOpen_) {
        close();
    }
}

// ============================================================
// 打开算子（初始化）
// ============================================================
// 初始化步骤：
// 1. 打开左右子算子
// 2. 加载右表所有行到内存（物化）
// 3. 如果是RIGHT JOIN，初始化匹配标志数组
// 4. 构建输出列名（带表名前缀，如users.id）
// ============================================================
Result<void> NestedLoopJoinOperator::open() {
    if (isOpen_) {
        return Result<void>();
    }

    // 打开左右子算子
    auto openLeft = left_->open();
    if (!openLeft.isSuccess()) {
        return openLeft;
    }

    auto openRight = right_->open();
    if (!openRight.isSuccess()) {
        return openRight;
    }

    // 加载右表所有行到内存（避免重复扫描右表）
    auto loadResult = loadRightTable();
    if (!loadResult.isSuccess()) {
        return loadResult;
    }

    LOG_INFO("NestedLoopJoinOperator", "Loaded " + std::to_string(rightRows_.size()) + " rows from right table");

    // 初始化RIGHT JOIN的匹配标志数组（用于阶段2）
    if (joinType_ == JoinType::RIGHT) {
        rightRowsMatched_.resize(rightRows_.size(), false);  // 标记哪些右行被匹配过
    }

    // 构建输出列名和类型（带表名前缀，避免列名冲突）
    std::string leftTableName = left_->getTableName();
    std::string rightTableName = right_->getTableName();

    auto leftNames = left_->getColumnNames();
    auto rightNames = right_->getColumnNames();

    // 左表列名：添加表名前缀（如users.name）
    for (const auto& name : leftNames) {
        if (!leftTableName.empty()) {
            columnNames_.push_back(leftTableName + "." + name);
        } else {
            columnNames_.push_back(name);
        }
    }

    // 右表列名：添加表名前缀（如orders.amount）
    for (const auto& name : rightNames) {
        if (!rightTableName.empty()) {
            columnNames_.push_back(rightTableName + "." + name);
        } else {
            columnNames_.push_back(name);
        }
    }

    auto leftTypes = left_->getColumnTypes();
    auto rightTypes = right_->getColumnTypes();
    columnTypes_.insert(columnTypes_.end(), leftTypes.begin(), leftTypes.end());
    columnTypes_.insert(columnTypes_.end(), rightTypes.begin(), rightTypes.end());

    isOpen_ = true;
    LOG_INFO("NestedLoopJoinOperator", "Opened nested loop join operator");
    return Result<void>();
}

// ============================================================
// 获取下一行连接结果（核心算法）
// ============================================================
// 嵌套循环连接算法：
// 阶段1：正常连接
//   while 有下一左行:
//     while 有下一右行:
//       if 连接条件匹配:
//         输出连接结果
//         标记左行和右行已匹配
//     if LEFT JOIN 且左行未匹配:
//       输出左行+NULL右行
//     移动到下一左行
//
// 阶段2（仅RIGHT JOIN）：输出未匹配的右行
//   for 每个右行:
//     if 右行未匹配:
//       输出NULL左行+右行
//
// 实现细节：
// - currentLeftRow_: 当前处理的左行
// - rightRowIndex_: 当前扫描的右行索引
// - leftRowMatched_: 当前左行是否匹配过（用于LEFT JOIN）
// - rightRowsMatched_: 标记哪些右行被匹配（用于RIGHT JOIN）
// ============================================================
Result<std::optional<Tuple>> NestedLoopJoinOperator::next() {
    if (!isOpen_) {
        MiniSQLException error(ErrorCode::EXEC_TABLE_NOT_FOUND, "Operator not open");
        return Result<std::optional<Tuple>>(error);
    }

    // ===== RIGHT JOIN 第二阶段：输出未匹配的右行 =====
    if (joinType_ == JoinType::RIGHT && inRightJoinPhase2_) {
        while (rightRowsIndex_ < rightRows_.size()) {
            if (!rightRowsMatched_[rightRowsIndex_]) {
                // 找到未匹配的右行，生成NULL填充的左行
                const Tuple& rightRow = rightRows_[rightRowsIndex_];
                Tuple nullLeftRow(left_->getColumnTypes().size());
                for (size_t i = 0; i < nullLeftRow.size(); ++i) {
                    nullLeftRow[i] = Value();  // NULL值
                }

                Tuple joinedRow = joinRows(nullLeftRow, rightRow);
                rightRowsIndex_++;
                return Result<std::optional<Tuple>>(joinedRow);
            }
            rightRowsIndex_++;
        }
        return Result<std::optional<Tuple>>(std::nullopt); // EOF - 所有右行已处理
    }

    // ===== 第一阶段：正常的嵌套循环连接 =====
    while (true) {
        // 如果没有当前左行，获取下一左行
        if (!currentLeftRow_.has_value()) {
            auto nextLeft = left_->next();
            if (!nextLeft.isSuccess()) {
                return Result<std::optional<Tuple>>(nextLeft.getError());
            }

            auto leftRowOpt = nextLeft.getValue();
            if (!leftRowOpt->has_value()) {
                // 左表遍历完成
                if (joinType_ == JoinType::RIGHT) {
                    // RIGHT JOIN: 进入第二阶段，输出未匹配的右行
                    inRightJoinPhase2_ = true;
                    rightRowsIndex_ = 0;
                    return next();  // 递归调用进入第二阶段
                }
                return Result<std::optional<Tuple>>(std::nullopt); // EOF
            }

            currentLeftRow_ = leftRowOpt->value();  // 保存当前左行
            rightRowIndex_ = 0;                      // 从头开始扫描右表
            leftRowMatched_ = false;                 // 重置匹配标志
        }

        // 遍历右表所有行，尝试连接
        while (rightRowIndex_ < rightRows_.size()) {
            const Tuple& rightRow = rightRows_[rightRowIndex_];
            Tuple joinedRow = joinRows(currentLeftRow_.value(), rightRow);

            // 检查连接条件
            bool matched = false;
            if (joinCond_) {
                // 构建行上下文（包含左右表的列，支持带表名前缀）
                RowContext context = buildRowContext(currentLeftRow_.value(), rightRow);
                evaluator_.setRowContext(context);

                // 求值连接条件
                auto evalResult = evaluator_.evaluate(joinCond_);
                if (!evalResult.isSuccess()) {
                    return Result<std::optional<Tuple>>(evalResult.getError());
                }

                Value conditionResult = *evalResult.getValue();
                matched = !conditionResult.isNull() && conditionResult.getBool();  // 条件为真则匹配
            } else {
                // 没有连接条件，笛卡尔积（所有组合都输出）
                matched = true;
            }

            rightRowIndex_++;  // 移动到下一右行

            if (matched) {
                // 匹配成功，输出连接结果
                leftRowMatched_ = true;  // 标记左行已匹配
                if (joinType_ == JoinType::RIGHT) {
                    rightRowsMatched_[rightRowIndex_ - 1] = true;  // 标记右行被匹配
                }
                return Result<std::optional<Tuple>>(joinedRow);
            }
        }

        // 右表遍历完成，处理LEFT JOIN的未匹配情况
        if (joinType_ == JoinType::LEFT && !leftRowMatched_) {
            // 左外连接：如果左行未匹配任何右行，输出左行+NULL右行
            Tuple nullRightRow(right_->getColumnTypes().size());
            for (size_t i = 0; i < nullRightRow.size(); ++i) {
                nullRightRow[i] = Value();  // NULL值
            }

            Tuple joinedRow = joinRows(currentLeftRow_.value(), nullRightRow);
            currentLeftRow_.reset(); // 移动到下一左行
            leftRowMatched_ = false;

            return Result<std::optional<Tuple>>(joinedRow);
        }

        // 移动到下一左行
        currentLeftRow_.reset();
    }
}

Result<void> NestedLoopJoinOperator::close() {
    if (isOpen_) {
        left_->close();
        right_->close();
        rightRows_.clear();
        currentLeftRow_.reset();
        isOpen_ = false;
        LOG_INFO("NestedLoopJoinOperator", "Closed nested loop join operator");
    }
    return Result<void>();
}

std::vector<std::string> NestedLoopJoinOperator::getColumnNames() const {
    return columnNames_;
}

std::vector<DataType> NestedLoopJoinOperator::getColumnTypes() const {
    return columnTypes_;
}

std::string NestedLoopJoinOperator::getTableName() const {
    return left_->getTableName();
}

// ============================================================
// 加载右表所有行到内存（辅助函数）
// ============================================================
// 物化右表的目的：
// - 嵌套循环连接需要多次扫描右表（每个左行扫描一次）
// - 将右表物化到内存避免重复打开/关闭右表算子
// - 内存访问比重新执行算子快得多
//
// 注意：如果右表很大，会占用大量内存
//       实际数据库会使用更高效的连接算法（如Hash Join）
// ============================================================
Result<void> NestedLoopJoinOperator::loadRightTable() {
    while (true) {
        auto nextResult = right_->next();
        if (!nextResult.isSuccess()) {
            return Result<void>(nextResult.getError());
        }

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) {
            break; // EOF - 右表扫描完成
        }

        rightRows_.push_back(rowOpt->value());  // 保存右行到内存
    }

    return Result<void>();
}

// ============================================================
// 拼接左右行（辅助函数）
// ============================================================
// 连接结果的构建：左表列 + 右表列
// 输出列顺序与列名列表一致
// ============================================================
Tuple NestedLoopJoinOperator::joinRows(const Tuple& left, const Tuple& right) {
    Tuple result;
    result.insert(result.end(), left.begin(), left.end());   // 左表列
    result.insert(result.end(), right.begin(), right.end()); // 右表列
    return result;
}

// ============================================================
// 构建行上下文（辅助函数）
// ============================================================
// 行上下文：列名 -> 值的映射
// 支持两种列名格式：
// 1. 简短格式：不带表名前缀（如 "name"）
// 2. 完整格式：带表名前缀（如 "users.name"）
//
// 完整格式用于JOIN条件中区分不同表的列：
// 例如：WHERE users.id = orders.user_id
// ============================================================
RowContext NestedLoopJoinOperator::buildRowContext(const Tuple& left, const Tuple& right) {
    RowContext context;

    // 获取左表和右表的名称
    std::string leftTableName = left_->getTableName();
    std::string rightTableName = right_->getTableName();

    // 添加左表列（两种格式：短名称和完整名称）
    auto leftNames = left_->getColumnNames();
    for (size_t i = 0; i < leftNames.size() && i < left.size(); ++i) {
        // 简短格式（向后兼容，用于简单查询）
        context[leftNames[i]] = left[i];
        // 完整格式（带表名前缀，用于JOIN条件）
        if (!leftTableName.empty()) {
            context[leftTableName + "." + leftNames[i]] = left[i];
        }
    }

    // 添加右表列（两种格式）
    auto rightNames = right_->getColumnNames();
    for (size_t i = 0; i < rightNames.size() && i < right.size(); ++i) {
        // 简短格式
        context[rightNames[i]] = right[i];
        // 完整格式
        if (!rightTableName.empty()) {
            context[rightTableName + "." + rightNames[i]] = right[i];
        }
    }

    return context;
}

} // namespace executor
} // namespace minisql
