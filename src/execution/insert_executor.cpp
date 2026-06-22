#include "execution/insert_executor.h"

namespace minidb {

InsertExecutor::InsertExecutor(HeapFile* heap_file, BPlusTree* index, const Schema& schema,
                               StatsManager* stats_mgr, const std::string& table_name,
                               const std::vector<Tuple>& tuples)
    : heap_file_(heap_file), index_(index), schema_(schema),
      stats_mgr_(stats_mgr), table_name_(table_name),
      tuples_(tuples), inserted_count_(0), done_(false) {}

void InsertExecutor::Open() {
    inserted_count_ = 0;
    done_ = false;

    for (const auto& tuple : tuples_) {
        // Serialize tuple to bytes
        std::string data = schema_.SerializeTuple(tuple);

        // Insert into heap file
        RID rid;
        Status s = heap_file_->InsertRecord(data.data(), static_cast<uint16_t>(data.size()), rid);
        if (!s.ok()) continue;

        // Insert into B+ Tree index (primary key)
        if (index_) {
            int32_t pk = schema_.GetPrimaryKey(tuple);
            index_->Insert(pk, rid);
        }

        // Update statistics
        if (stats_mgr_) {
            int32_t pk = schema_.GetPrimaryKey(tuple);
            stats_mgr_->GetStats(table_name_).UpdateOnInsert(pk);
        }

        inserted_count_++;
    }
}

bool InsertExecutor::Next(Tuple& tuple, RID& rid) {
    if (done_) return false;
    done_ = true;
    // Return a single tuple with the count of inserted rows
    tuple = {inserted_count_};
    rid = RID();
    return true;
}

void InsertExecutor::Close() {}

}  // namespace minidb
