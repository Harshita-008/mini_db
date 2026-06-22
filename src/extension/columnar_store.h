#pragma once

#include "common/types.h"
#include "common/config.h"
#include "catalog/schema.h"
#include <string>
#include <vector>
#include <fstream>

namespace minidb {

// Column data — stores a single column's values contiguously
struct ColumnData {
    ColumnType type;
    std::vector<int32_t> int_data;
    std::vector<double> float_data;
    std::vector<std::string> string_data;
    std::vector<bool> bool_data;
    size_t size = 0;
};

// Columnar storage — stores table data in column-oriented format.
// Each column is stored as a separate contiguous array for cache-friendly access.
class ColumnarStore {
public:
    ColumnarStore(const std::string& table_name, const Schema& schema);

    // Load data from row-oriented tuples
    void LoadFromTuples(const std::vector<Tuple>& tuples);

    // Get a specific column's data
    const ColumnData& GetColumn(size_t col_idx) const { return columns_[col_idx]; }

    // Get number of rows
    size_t GetRowCount() const { return row_count_; }

    // Get schema
    const Schema& GetSchema() const { return schema_; }

    // Scan a single column (returns column data)
    const ColumnData& ScanColumn(size_t col_idx) const;

    // Scan multiple columns
    std::vector<const ColumnData*> ScanColumns(const std::vector<size_t>& col_indices) const;

    // Evaluate a filter on a column, returns matching row indices
    std::vector<size_t> FilterColumn(size_t col_idx, const std::string& op, const Value& value) const;

    // Compute SUM on an integer column
    int64_t SumInt(size_t col_idx) const;
    int64_t SumIntFiltered(size_t col_idx, const std::vector<size_t>& row_indices) const;

    // Compute COUNT
    size_t Count() const { return row_count_; }
    size_t CountFiltered(const std::vector<size_t>& row_indices) const { return row_indices.size(); }

    // Materialize rows from column data (for result output)
    Tuple MaterializeRow(size_t row_idx) const;
    Tuple MaterializeRow(size_t row_idx, const std::vector<size_t>& col_indices) const;

private:
    std::string table_name_;
    Schema schema_;
    std::vector<ColumnData> columns_;
    size_t row_count_ = 0;
};

}  // namespace minidb
