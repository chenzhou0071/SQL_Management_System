#include "TableScanOperator.h"
#include "../storage/TableManager.h"
#include "../common/Logger.h"

namespace minisql {
namespace executor {

TableScanOperator::TableScanOperator(const std::string& dbName, const std::string& tableName)
    : dbName_(dbName), tableName_(tableName), currentRowIndex_(0), isOpen_(false) {
}

TableScanOperator::~TableScanOperator() {
    if (isOpen_) {
        close();
    }
}

Result<void> TableScanOperator::open() {
    auto& tableMgr = storage::TableManager::getInstance();

    // 加载表定义
    auto tableDefResult = tableMgr.getTableDef(dbName_, tableName_);
    if (!tableDefResult.isSuccess()) {
        MiniSQLException error(ErrorCode::EXEC_TABLE_NOT_FOUND,
            "Table not found: " + dbName_ + "." + tableName_);
        return Result<void>(error);
    }
    tableDef_ = *tableDefResult.getValue();

    // 加载表数据
    auto tableDataResult = tableMgr.loadTable(dbName_, tableName_);
    if (!tableDataResult.isSuccess()) {
        MiniSQLException error(ErrorCode::EXEC_TABLE_NOT_FOUND,
            "Failed to load table data: " + tableName_);
        return Result<void>(error);
    }
    tableData_ = *tableDataResult.getValue();

    currentRowIndex_ = 0;
    isOpen_ = true;

    LOG_INFO("TableScan", "Opened table scan: " + dbName_ + "." + tableName_);
    return Result<void>();
}

Result<std::optional<Tuple>> TableScanOperator::next() {
    if (!isOpen_) {
        MiniSQLException error(ErrorCode::EXEC_TABLE_NOT_FOUND, "Operator not open");
        return Result<std::optional<Tuple>>(error);
    }

    if (currentRowIndex_ >= tableData_.size()) {
        return Result<std::optional<Tuple>>(std::nullopt); // EOF
    }

    // 获取当前行
    auto it = tableData_.begin();
    std::advance(it, currentRowIndex_);
    Tuple row = it->second;

    currentRowIndex_++;
    return Result<std::optional<Tuple>>(row);
}

Result<void> TableScanOperator::close() {
    tableData_.clear();
    isOpen_ = false;
    LOG_INFO("TableScan", "Closed table scan: " + dbName_ + "." + tableName_);
    return Result<void>();
}

std::vector<std::string> TableScanOperator::getColumnNames() const {
    std::vector<std::string> names;
    for (const auto& col : tableDef_.columns) {
        names.push_back(col.name);
    }
    return names;
}

std::vector<DataType> TableScanOperator::getColumnTypes() const {
    std::vector<DataType> types;
    for (const auto& col : tableDef_.columns) {
        types.push_back(col.type);
    }
    return types;
}

std::string TableScanOperator::getTableName() const {
    return tableName_;
}

} // namespace executor
} // namespace minisql
