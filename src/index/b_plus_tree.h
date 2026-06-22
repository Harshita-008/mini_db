#pragma once

#include "common/types.h"
#include "common/rid.h"
#include "common/status.h"
#include "storage/buffer_pool.h"
#include "index/b_plus_tree_node.h"
#include <vector>
#include <mutex>

namespace minidb {

// B+ Tree index with int32_t keys mapped to RIDs.
class BPlusTree {
public:
    BPlusTree(BufferPool* buffer_pool, page_id_t root_page_id = INVALID_PAGE_ID);

    // Create a new B+ tree, returns root page id
    page_id_t Create();

    // Point lookup: find the RID for a given key
    Status Search(int32_t key, RID& rid);

    // Insert a key-RID pair
    Status Insert(int32_t key, const RID& rid);

    // Delete a key
    Status Delete(int32_t key);

    // Range scan: find all RIDs with keys in [low_key, high_key]
    std::vector<RID> RangeScan(int32_t low_key, int32_t high_key);

    // Get all key-RID pairs (for debugging / full scan)
    std::vector<std::pair<int32_t, RID>> GetAll();

    page_id_t GetRootPageId() const { return root_page_id_; }
    bool IsEmpty() const { return root_page_id_ == INVALID_PAGE_ID; }

private:
    // Find the leaf page that should contain the given key
    page_id_t FindLeafPage(int32_t key);

    // Split a full leaf node
    void SplitLeaf(page_id_t leaf_page_id);

    // Split a full internal node
    void SplitInternal(page_id_t internal_page_id);

    // Insert a key into parent internal node after a child split
    void InsertIntoParent(page_id_t left_page_id, int32_t key, page_id_t right_page_id);

    BufferPool* buffer_pool_;
    page_id_t root_page_id_;
    std::mutex mutex_;
};

}  // namespace minidb
