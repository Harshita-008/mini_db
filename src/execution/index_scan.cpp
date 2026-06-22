#include "execution/index_scan.h"

namespace minidb {

IndexScanExecutor::IndexScanExecutor(BPlusTree* index, HeapFile* heap_file,
                                     const Schema& schema, int32_t key)
    : index_(index), heap_file_(heap_file), schema_(schema), key_(key),
      found_(false), consumed_(false) {}

void IndexScanExecutor::Open() {
    found_ = false;
    consumed_ = false;

    RID rid;
    Status s = index_->Search(key_, rid);
    if (s.ok()) {
        // Fetch the actual record from heap file
        char data[MAX_RECORD_SIZE];
        uint16_t length;
        Status rs = heap_file_->GetRecord(rid, data, length);
        if (rs.ok()) {
            result_rid_ = rid;
            result_tuple_ = schema_.DeserializeTuple(data, length);
            found_ = true;
        }
    }
}

bool IndexScanExecutor::Next(Tuple& tuple, RID& rid) {
    if (!found_ || consumed_) return false;
    tuple = result_tuple_;
    rid = result_rid_;
    consumed_ = true;
    return true;
}

void IndexScanExecutor::Close() {
    found_ = false;
    consumed_ = false;
}

}  // namespace minidb
