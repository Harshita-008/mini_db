#pragma once

#include "common/config.h"
#include "common/types.h"
#include "common/status.h"
#include <string>
#include <fstream>
#include <mutex>

namespace minidb {

// PageManager handles raw disk I/O for database files.
// Each file is organized as a sequence of fixed-size pages (PAGE_SIZE bytes).
class PageManager {
public:
    explicit PageManager(const std::string& file_path);
    ~PageManager();

    // Read a page from disk into buffer
    Status ReadPage(page_id_t page_id, char* data);

    // Write a page from buffer to disk
    Status WritePage(page_id_t page_id, const char* data);

    // Allocate a new page, returns its page_id
    page_id_t AllocatePage();

    // Get total number of pages in file
    page_id_t GetNumPages() const { return num_pages_; }

    const std::string& GetFilePath() const { return file_path_; }

private:
    std::string file_path_;
    std::fstream file_;
    page_id_t num_pages_;
    std::mutex mutex_;
};

}  // namespace minidb
