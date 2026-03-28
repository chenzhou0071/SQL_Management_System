#include "ExplainHandler.h"
#include "PlanGenerator.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

using namespace minisql;
using namespace optimizer;

std::string ExplainRow::toString() const {
    std::ostringstream oss;
    oss << "id=" << id
        << ", type=" << type
        << ", table=" << table
        << ", key=" << key
        << ", rows=" << rows;
    return oss.str();
}

std::string ExplainHandler::getAccessType(ScanType scanType, bool hasEquality, double selectivity) {
    switch (scanType) {
        case ScanType::FULL_SCAN:
            return "ALL";
        case ScanType::COVERING_SCAN:
            return hasEquality ? "ref" : "range";
        case ScanType::INDEX_SCAN:
            if (selectivity < 0.01) return "const";
            if (hasEquality) return "eq_ref";
            return "range";
        default:
            return "ALL";
    }
}

std::string ExplainHandler::buildExtra(const PlanNode* node) {
    std::vector<std::string> extras;

    if (!node) return "";

    // 检查是否使用了覆盖索引
    if (node->scanType == ScanType::COVERING_SCAN) {
        extras.push_back("Using index");
    }

    // 检查是否有排序
    if (node->operatorType == "Sort") {
        extras.push_back("Using filesort");
    }

    // 检查是否有临时表
    if (node->operatorType == "Aggregate" && !node->groupBy.empty()) {
        extras.push_back("Using temporary");
    }

    // 连接操作的额外信息
    if (node->operatorType == "NestedLoopJoin") {
        extras.push_back("Using join buffer");
    }

    // 构建字符串
    std::ostringstream oss;
    for (size_t i = 0; i < extras.size(); ++i) {
        if (i > 0) oss << "; ";
        oss << extras[i];
    }
    return oss.str();
}

void ExplainHandler::collectExplainRows(const PlanNode* node, int id, std::vector<ExplainRow>& rows) {
    if (!node) return;

    ExplainRow row;
    row.id = id;
    row.selectType = "SIMPLE";
    row.table = node->tableName;
    row.rows = static_cast<int>(node->estimatedRows);

    // 设置访问类型
    bool hasEquality = false;
    double selectivity = node->estimatedRows > 0 ? 0.1 : 0.1;
    row.type = getAccessType(node->scanType, hasEquality, selectivity);

    row.key = node->indexName;
    row.possibleKeys = node->indexName;

    // 添加 Extra 信息
    std::string extra = buildExtra(node);
    if (!extra.empty()) {
        row.extra.push_back(extra);
    }

    rows.push_back(row);

    // 递归处理子节点
    int childId = id + 1;
    for (const auto& child : node->children) {
        collectExplainRows(child.get(), childId, rows);
        childId++;
    }
}

std::vector<ExplainRow> ExplainHandler::explainPlan(std::shared_ptr<PlanNode> plan) {
    std::vector<ExplainRow> rows;
    if (plan) {
        collectExplainRows(plan.get(), 1, rows);
    }
    return rows;
}

Result<std::vector<ExplainRow>> ExplainHandler::explain(
    const std::string& dbName,
    parser::SelectStmt* stmt) {

    if (!stmt) {
        return Result<std::vector<ExplainRow>>(MiniSQLException(
            ErrorCode::EXEC_INVALID_VALUE, "NULL statement"));
    }

    // 生成执行计划
    auto planResult = PlanGenerator::generate(dbName, stmt);
    if (planResult.isError()) {
        return Result<std::vector<ExplainRow>>(planResult.getError());
    }

    auto plan = *planResult.getValue();
    auto rows = explainPlan(plan);
    return Result<std::vector<ExplainRow>>(rows);
}

std::string ExplainHandler::formatTable(const std::vector<ExplainRow>& rows) {
    if (rows.empty()) return "";

    std::ostringstream oss;

    // 计算列宽
    const int colWidths[] = {3, 10, 12, 8, 15, 8, 6, 30};
    const std::string headers[] = {"id", "select_type", "table", "type", "possible_keys", "key", "rows", "Extra"};

    // 表头
    oss << "+";
    for (int w : colWidths) {
        oss << std::string(w, '-') << "+";
    }
    oss << "\n";

    oss << "|";
    for (int i = 0; i < 8; ++i) {
        oss << std::setw(colWidths[i]) << std::left << headers[i] << "|";
    }
    oss << "\n";

    oss << "+";
    for (int w : colWidths) {
        oss << std::string(w, '-') << "+";
    }
    oss << "\n";

    // 数据行
    for (const auto& row : rows) {
        oss << "|";
        oss << std::setw(colWidths[0]) << std::left << std::to_string(row.id) << "|";
        oss << std::setw(colWidths[1]) << std::left << row.selectType << "|";
        oss << std::setw(colWidths[2]) << std::left << row.table << "|";
        oss << std::setw(colWidths[3]) << std::left << row.type << "|";
        oss << std::setw(colWidths[4]) << std::left << row.possibleKeys << "|";
        oss << std::setw(colWidths[5]) << std::left << row.key << "|";
        oss << std::setw(colWidths[6]) << std::left << std::to_string(row.rows) << "|";

        std::string extraStr;
        for (const auto& e : row.extra) {
            if (!extraStr.empty()) extraStr += "; ";
            extraStr += e;
        }
        oss << std::setw(colWidths[7]) << std::left << extraStr << "|\n";
    }

    // 表尾
    oss << "+";
    for (int w : colWidths) {
        oss << std::string(w, '-') << "+";
    }
    oss << "\n";

    return oss.str();
}

std::string ExplainHandler::formatJSON(const std::vector<ExplainRow>& rows) {
    std::ostringstream oss;
    oss << "[\n";

    for (size_t i = 0; i < rows.size(); ++i) {
        const auto& row = rows[i];
        oss << "  {\n";
        oss << "    \"id\": " << row.id << ",\n";
        oss << "    \"select_type\": \"" << row.selectType << "\",\n";
        oss << "    \"table\": \"" << row.table << "\",\n";
        oss << "    \"type\": \"" << row.type << "\",\n";
        oss << "    \"possible_keys\": \"" << row.possibleKeys << "\",\n";
        oss << "    \"key\": \"" << row.key << "\",\n";
        oss << "    \"rows\": " << row.rows;

        if (!row.extra.empty()) {
            oss << ",\n    \"Extra\": [";
            for (size_t j = 0; j < row.extra.size(); ++j) {
                oss << "\"" << row.extra[j] << "\"";
                if (j < row.extra.size() - 1) oss << ", ";
            }
            oss << "]";
        }

        oss << "\n  }";
        if (i < rows.size() - 1) oss << ",";
        oss << "\n";
    }

    oss << "]\n";
    return oss.str();
}
