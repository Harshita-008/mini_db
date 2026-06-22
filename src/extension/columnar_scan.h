#pragma once

#include "execution/executor.h"
#include "extension/columnar_store.h"
#include <vector>

namespace minidb {

// Columnar scan executor — reads from columnar storage with batch processing.
class ColumnarScanExecutor : public Executor {
public:
    ColumnarScanExecutor(const ColumnarStore* store, const std::vector<size_t>& col_indices);

    void Open() override;
    bool Next(Tuple& tuple, RID& rid) override;
    void Close() override;

    // Set filter: col_idx, operator, value
    void SetFilter(size_t col_idx, const std::string& op, const Value& value);

private:
    const ColumnarStore* store_;
    std::vector<size_t> col_indices_;
    size_t cursor_;

    bool has_filter_ = false;
    size_t filter_col_;
    std::string filter_op_;
    Value filter_value_;
    std::vector<size_t> matching_rows_;
};

}  // namespace minidb
