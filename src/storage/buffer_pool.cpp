#include "storage/buffer_pool.h"
#include <cassert>
#include <cstring>

namespace minidb {

BufferPool::BufferPool(PageManager* page_manager, size_t pool_size)
    : page_manager_(page_manager), pool_size_(pool_size) {
    frames_.resize(pool_size);

    // Initialize all frames as free
    for (frame_id_t i = 0; i < pool_size; i++) {
        free_list_.push_back(i);
    }
}

BufferPool::~BufferPool() {
    FlushAllPages();
}

Page* BufferPool::FetchPage(page_id_t page_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if page is already in buffer pool
    auto it = page_table_.find(page_id);
    if (it != page_table_.end()) {
        frame_id_t frame_id = it->second;
        FrameInfo& frame = frames_[frame_id];
        frame.pin_count++;

        // Remove from LRU list if present (pinned pages aren't in LRU)
        auto lru_it = lru_map_.find(frame_id);
        if (lru_it != lru_map_.end()) {
            lru_list_.erase(lru_it->second);
            lru_map_.erase(lru_it);
        }

        return &frame.page;
    }

    // Page not in pool — need to fetch from disk
    frame_id_t frame_id = FindVictim();
    if (frame_id == INVALID_FRAME_ID) {
        return nullptr;  // No available frame
    }

    FrameInfo& frame = frames_[frame_id];

    // If victim frame has a dirty page, flush it first
    if (frame.page_id != INVALID_PAGE_ID) {
        if (frame.is_dirty) {
            page_manager_->WritePage(frame.page_id, frame.page.GetData());
        }
        page_table_.erase(frame.page_id);
    }

    // Read new page from disk
    Status s = page_manager_->ReadPage(page_id, frame.page.GetData());
    if (!s.ok()) {
        // If page doesn't exist on disk yet, initialize it
        frame.page.Init(page_id);
    }

    // Setup frame
    frame.page_id = page_id;
    frame.is_dirty = false;
    frame.pin_count = 1;
    page_table_[page_id] = frame_id;

    // Remove from LRU if it was there
    auto lru_it = lru_map_.find(frame_id);
    if (lru_it != lru_map_.end()) {
        lru_list_.erase(lru_it->second);
        lru_map_.erase(lru_it);
    }

    return &frame.page;
}

Page* BufferPool::NewPage(page_id_t& page_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    frame_id_t frame_id = FindVictim();
    if (frame_id == INVALID_FRAME_ID) {
        return nullptr;
    }

    FrameInfo& frame = frames_[frame_id];

    // If victim has a dirty page, flush it
    if (frame.page_id != INVALID_PAGE_ID) {
        if (frame.is_dirty) {
            page_manager_->WritePage(frame.page_id, frame.page.GetData());
        }
        page_table_.erase(frame.page_id);
    }

    // Allocate new page on disk
    page_id = page_manager_->AllocatePage();

    // Initialize the page
    frame.page.Init(page_id);
    frame.page_id = page_id;
    frame.is_dirty = true;  // New page is dirty
    frame.pin_count = 1;
    page_table_[page_id] = frame_id;

    return &frame.page;
}

bool BufferPool::UnpinPage(page_id_t page_id, bool is_dirty) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) {
        return false;
    }

    frame_id_t frame_id = it->second;
    FrameInfo& frame = frames_[frame_id];

    if (frame.pin_count <= 0) {
        return false;
    }

    frame.pin_count--;
    if (is_dirty) {
        frame.is_dirty = true;
    }

    // If pin count reaches 0, add to LRU list (eligible for eviction)
    if (frame.pin_count == 0) {
        lru_list_.push_front(frame_id);
        lru_map_[frame_id] = lru_list_.begin();
    }

    return true;
}

Status BufferPool::FlushPage(page_id_t page_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) {
        return Status::NotFound("Page not in buffer pool");
    }

    frame_id_t frame_id = it->second;
    FrameInfo& frame = frames_[frame_id];

    if (frame.is_dirty) {
        Status s = page_manager_->WritePage(page_id, frame.page.GetData());
        if (!s.ok()) return s;
        frame.is_dirty = false;
    }

    return Status::OK();
}

void BufferPool::FlushAllPages() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& [page_id, frame_id] : page_table_) {
        FrameInfo& frame = frames_[frame_id];
        if (frame.is_dirty) {
            page_manager_->WritePage(page_id, frame.page.GetData());
            frame.is_dirty = false;
        }
    }
}

bool BufferPool::DeletePage(page_id_t page_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) {
        return true;  // Not in pool, nothing to do
    }

    frame_id_t frame_id = it->second;
    FrameInfo& frame = frames_[frame_id];

    if (frame.pin_count > 0) {
        return false;  // Can't delete pinned page
    }

    // Remove from LRU
    auto lru_it = lru_map_.find(frame_id);
    if (lru_it != lru_map_.end()) {
        lru_list_.erase(lru_it->second);
        lru_map_.erase(lru_it);
    }

    // Reset frame
    page_table_.erase(it);
    frame.page_id = INVALID_PAGE_ID;
    frame.is_dirty = false;
    frame.pin_count = 0;
    free_list_.push_back(frame_id);

    return true;
}

frame_id_t BufferPool::FindVictim() {
    // First check free list
    if (!free_list_.empty()) {
        frame_id_t frame_id = free_list_.front();
        free_list_.pop_front();
        return frame_id;
    }

    // Use LRU — evict from back (least recently used)
    if (!lru_list_.empty()) {
        frame_id_t frame_id = lru_list_.back();
        lru_list_.pop_back();
        lru_map_.erase(frame_id);
        return frame_id;
    }

    // All pages are pinned — can't evict anything
    return INVALID_FRAME_ID;
}

}  // namespace minidb
