#include "execution/delete_executor.h"

namespace minidb {

DeleteExecutor::DeleteExecutor(std::unique_ptr<Executor> child, HeapFile* heap_file,
                               BPlusTree* index, const Schema& schema,
                               StatsManager* stats_mgr, const std::string& table_name)
    : child_(std::move(child)), heap_file_(heap_file), index_(index),
      schema_(schema), stats_mgr_(stats_mgr), table_name_(table_name),
      deleted_count_(0), done_(false) {}

void DeleteExecutor::Open() {
    child_->Open();
    deleted_count_ = 0;
    done_ = false;

    // Collect all tuples to delete first (to avoid modifying during scan)
    std::vector<std::pair<RID, Tuple>> to_delete;
    Tuple tuple;
    RID rid;
    while (child_->Next(tuple, rid)) {
        to_delete.push_back({rid, tuple});
    }

    // Now delete them
    for (const auto& [del_rid, del_tuple] : to_delete) {
        // Delete from heap file
        Status s = heap_file_->DeleteRecord(del_rid);
        if (!s.ok()) continue;

        // Delete from B+ Tree index
        if (index_) {
            int32_t pk = schema_.GetPrimaryKey(del_tuple);
            index_->Delete(pk);
        }

        // Update statistics
        if (stats_mgr_) {
            stats_mgr_->GetStats(table_name_).UpdateOnDelete();
        }

        deleted_count_++;
    }
}

bool DeleteExecutor::Next(Tuple& tuple, RID& rid) {
    if (done_) return false;
    done_ = true;
    tuple = {deleted_count_};
    rid = RID();
    return true;
}

void DeleteExecutor::Close() {
    child_->Close();
}

}  // namespace minidb
