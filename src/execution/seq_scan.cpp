#include "execution/seq_scan.h"

namespace minidb {

SeqScanExecutor::SeqScanExecutor(HeapFile* heap_file, const Schema& schema)
    : heap_file_(heap_file), schema_(schema), cursor_(0) {}

void SeqScanExecutor::Open() {
    results_.clear();
    cursor_ = 0;

    heap_file_->Scan([this](const RID& rid, const char* data, uint16_t length) -> bool {
        Tuple tuple = schema_.DeserializeTuple(data, length);
        results_.push_back({rid, std::move(tuple)});
        return true;  // continue scanning
    });
}

bool SeqScanExecutor::Next(Tuple& tuple, RID& rid) {
    if (cursor_ >= results_.size()) return false;
    rid = results_[cursor_].first;
    tuple = results_[cursor_].second;
    cursor_++;
    return true;
}

void SeqScanExecutor::Close() {
    results_.clear();
    cursor_ = 0;
}

}  // namespace minidb
