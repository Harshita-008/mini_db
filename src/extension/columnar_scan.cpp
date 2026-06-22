#include "extension/columnar_scan.h"

namespace minidb {

ColumnarScanExecutor::ColumnarScanExecutor(const ColumnarStore* store,
                                           const std::vector<size_t>& col_indices)
    : store_(store), col_indices_(col_indices), cursor_(0) {}

void ColumnarScanExecutor::Open() {
    cursor_ = 0;
    if (has_filter_) {
        matching_rows_ = store_->FilterColumn(filter_col_, filter_op_, filter_value_);
    }
}

bool ColumnarScanExecutor::Next(Tuple& tuple, RID& rid) {
    if (has_filter_) {
        if (cursor_ >= matching_rows_.size()) return false;
        size_t row_idx = matching_rows_[cursor_];
        tuple = store_->MaterializeRow(row_idx, col_indices_);
        rid = RID(0, static_cast<slot_num_t>(row_idx));
        cursor_++;
        return true;
    } else {
        if (cursor_ >= store_->GetRowCount()) return false;
        tuple = store_->MaterializeRow(cursor_, col_indices_);
        rid = RID(0, static_cast<slot_num_t>(cursor_));
        cursor_++;
        return true;
    }
}

void ColumnarScanExecutor::Close() {
    cursor_ = 0;
    matching_rows_.clear();
}

void ColumnarScanExecutor::SetFilter(size_t col_idx, const std::string& op, const Value& value) {
    has_filter_ = true;
    filter_col_ = col_idx;
    filter_op_ = op;
    filter_value_ = value;
}

}  // namespace minidb
