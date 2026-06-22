#pragma once

#include "execution/executor.h"
#include "storage/heap_file.h"
#include "catalog/schema.h"

namespace minidb {

// Sequential scan — iterates over all records in a heap file.
class SeqScanExecutor : public Executor {
public:
    SeqScanExecutor(HeapFile* heap_file, const Schema& schema);

    void Open() override;
    bool Next(Tuple& tuple, RID& rid) override;
    void Close() override;

private:
    HeapFile* heap_file_;
    Schema schema_;

    // Collected results (we scan once and buffer)
    std::vector<std::pair<RID, Tuple>> results_;
    size_t cursor_;
};

}  // namespace minidb
