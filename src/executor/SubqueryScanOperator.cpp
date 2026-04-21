#include "SubqueryScanOperator.h"
#include "../common/Logger.h"

namespace minisql {
namespace executor {

// ============================================================
// 子查询扫描算子 - 处理FROM子句中的子查询
// ============================================================
// 用途示例：
//   SELECT * FROM (SELECT id, name FROM users WHERE age > 18) AS young_users
//
// 子查询在FROM子句中的作用：
// - 子查询的结果作为临时表（派生表）
// - 可以使用别名（如AS young_users）引用子查询结果
// - 外层查询可以像操作普通表一样操作子查询结果
//
// 实现策略：
// - 物化模式：一次性执行子查询，将所有结果缓存到内存
// - 后续扫描直接访问内存中的物化结果
// - 优点：子查询只执行一次，避免重复执行
// - 缺点：如果子查询结果很大，占用内存多
//
// 注意事项：
// - 子查询必须先执行并物化，外层查询才能开始
// - 列名添加别名前缀（如young_users.id）
// - 子查询算子在open()后立即关闭，释放资源
// ============================================================

SubqueryScanOperator::SubqueryScanOperator(OperatorPtr subqueryOp, const std::string& alias)
    : subqueryOp_(subqueryOp), alias_(alias), isOpen_(false), currentRowIndex_(0) {
    children_.push_back(subqueryOp);
}

SubqueryScanOperator::~SubqueryScanOperator() {
    if (isOpen_) {
        close();
    }
}

// ============================================================
// 打开算子（执行子查询并物化结果）
// ============================================================
// 初始化步骤：
// 1. 打开子查询算子
// 2. 执行子查询，读取所有结果到内存（物化）
// 3. 关闭子查询算子（释放底层资源）
// 4. 构建输出列名（添加别名前缀）
// 5. 准备逐行扫描物化结果
//
// 物化目的：
// - 子查询只执行一次，避免外层查询每行都重新执行子查询
// - 将子查询结果视为临时表，存储在内存中
// - 外层查询可以多次扫描这个临时表
// ============================================================
Result<void> SubqueryScanOperator::open() {
    if (isOpen_) {
        return Result<void>();
    }

    // 打开子查询算子
    auto openResult = subqueryOp_->open();
    if (!openResult.isSuccess()) {
        return openResult;
    }

    // 物化子查询的所有结果到内存
    while (true) {
        auto nextResult = subqueryOp_->next();
        if (!nextResult.isSuccess()) {
            return Result<void>(nextResult.getError());
        }

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) {
            break; // EOF - 子查询执行完成
        }

        materializedRows_.push_back(rowOpt->value());  // 缓存子查询结果行
    }

    // 关闭子查询算子（释放底层资源，如表扫描、文件句柄）
    subqueryOp_->close();

    // 构建列名和类型（添加别名前缀，避免列名冲突）
    auto subNames = subqueryOp_->getColumnNames();
    auto subTypes = subqueryOp_->getColumnTypes();

    // 如果有别名，添加别名前缀（如young_users.id）
    if (!alias_.empty()) {
        for (const auto& name : subNames) {
            columnNames_.push_back(alias_ + "." + name);
        }
    } else {
        columnNames_ = subNames;  // 无别名，使用子查询原始列名
    }

    columnTypes_ = subTypes;  // 列类型与子查询一致

    currentRowIndex_ = 0;  // 初始化扫描索引
    isOpen_ = true;

    LOG_INFO("SubqueryScanOperator", "Opened subquery scan: " + alias_ + " with " + std::to_string(materializedRows_.size()) + " rows");
    return Result<void>();
}

// ============================================================
// 获取下一行（扫描物化结果）
// ============================================================
// 简单的顺序扫描：
// - 从内存中逐行返回物化的子查询结果
// - currentRowIndex_跟踪当前扫描位置
// - 扫描完成返回nullopt
//
// 优点：扫描速度快（内存访问）
// 缺点：无法利用索引（物化结果未建索引）
// ============================================================
Result<std::optional<Tuple>> SubqueryScanOperator::next() {
    if (!isOpen_) {
        MiniSQLException error(ErrorCode::EXEC_TABLE_NOT_FOUND, "Operator not open");
        return Result<std::optional<Tuple>>(error);
    }

    // 检查是否扫描完成
    if (currentRowIndex_ >= materializedRows_.size()) {
        return Result<std::optional<Tuple>>(std::nullopt); // EOF
    }

    // 返回当前行并移动索引
    Tuple row = materializedRows_[currentRowIndex_];
    currentRowIndex_++;
    return Result<std::optional<Tuple>>(row);
}

Result<void> SubqueryScanOperator::close() {
    if (isOpen_) {
        materializedRows_.clear();
        currentRowIndex_ = 0;
        isOpen_ = false;
        LOG_INFO("SubqueryScanOperator", "Closed subquery scan: " + alias_);
    }
    return Result<void>();
}

std::vector<std::string> SubqueryScanOperator::getColumnNames() const {
    return columnNames_;
}

std::vector<DataType> SubqueryScanOperator::getColumnTypes() const {
    return columnTypes_;
}

std::string SubqueryScanOperator::getTableName() const {
    return alias_;
}

} // namespace executor
} // namespace minisql
