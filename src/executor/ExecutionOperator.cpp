#include "ExecutionOperator.h"

namespace minisql {
namespace executor {

// ExecutionResult implementation
size_t ExecutionResult::getRowCount() const {
    return rows.size();
}

size_t ExecutionResult::getColumnCount() const {
    return columnNames.size();
}

} // namespace executor
} // namespace minisql
