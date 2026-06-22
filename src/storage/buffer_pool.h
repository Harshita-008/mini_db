#pragma once

#include "common/config.h"
#include "common/types.h"
#include "common/status.h"
#include "storage/page.h"
#include "storage/page_manager.h"
#include <list>
#include <unordered_map>
#include <mutex>
#include <memory>

namespace minidb {

// BufferPool manages a fixed-size pool of in-memory page frames.
// Uses LRU replacement policy.
class BufferPool {
public:
    explicit BufferPool(PageManager* page_manager, size_t pool_size = BUFFER_POOL_SIZE);
    ~BufferPool();

    // Fetch a page from the buffer pool (reads from disk if not cached)
    // Returns pinned page — caller MUST call UnpinPage when done.
    Page* FetchPage(page_id_t page_id);

    // Create a new page in the buffer pool
    Page* NewPage(page_id_t& page_id);

    // Unpin a page (decrements pin count, optionally marks dirty)
    bool UnpinPage(page_id_t page_id, bool is_dirty);

    // Flush a specific page to disk
    Status FlushPage(page_id_t page_id);

    // Flush all dirty pages to disk
    void FlushAllPages();

    // Delete a page from the buffer pool
    bool DeletePage(page_id_t page_id);

    PageManager* GetPageManager() { return page_manager_; }

private:
    // Find a victim frame for eviction using LRU
    frame_id_t FindVictim();

    struct FrameInfo {
        Page page;
        page_id_t page_id = INVALID_PAGE_ID;
        bool is_dirty = false;
        int pin_count = 0;
    };

    PageManager* page_manager_;
    size_t pool_size_;
    std::vector<FrameInfo> frames_;

    // Page table: page_id -> frame_id
    std::unordered_map<page_id_t, frame_id_t> page_table_;

    // LRU list: front = most recently used, back = least recently used
    std::list<frame_id_t> lru_list_;
    std::unordered_map<frame_id_t, std::list<frame_id_t>::iterator> lru_map_;

    // Free frames list
    std::list<frame_id_t> free_list_;

    std::mutex mutex_;
};

}  // namespace minidb
