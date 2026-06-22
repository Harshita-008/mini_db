#include "storage/page_manager.h"
#include <filesystem>
#include <cstring>

namespace minidb {

PageManager::PageManager(const std::string& file_path) : file_path_(file_path), num_pages_(0) {
    // Ensure directory exists
    auto parent = std::filesystem::path(file_path).parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }

    // Open or create the file
    file_.open(file_path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file_.is_open()) {
        // File doesn't exist, create it
        file_.clear();
        file_.open(file_path, std::ios::out | std::ios::binary);
        file_.close();
        file_.open(file_path, std::ios::in | std::ios::out | std::ios::binary);
    }

    if (file_.is_open()) {
        file_.seekg(0, std::ios::end);
        auto file_size = file_.tellg();
        num_pages_ = static_cast<page_id_t>(file_size / PAGE_SIZE);
    }
}

PageManager::~PageManager() {
    if (file_.is_open()) {
        file_.close();
    }
}

Status PageManager::ReadPage(page_id_t page_id, char* data) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (page_id >= num_pages_) {
        return Status::IOError("Page ID " + std::to_string(page_id) + " out of range");
    }

    file_.seekg(static_cast<std::streamoff>(page_id) * PAGE_SIZE);
    file_.read(data, PAGE_SIZE);

    if (!file_.good() && !file_.eof()) {
        file_.clear();
        return Status::IOError("Failed to read page " + std::to_string(page_id));
    }
    file_.clear();

    return Status::OK();
}

Status PageManager::WritePage(page_id_t page_id, const char* data) {
    std::lock_guard<std::mutex> lock(mutex_);

    file_.seekp(static_cast<std::streamoff>(page_id) * PAGE_SIZE);
    file_.write(data, PAGE_SIZE);
    file_.flush();

    if (!file_.good()) {
        file_.clear();
        return Status::IOError("Failed to write page " + std::to_string(page_id));
    }

    return Status::OK();
}

page_id_t PageManager::AllocatePage() {
    std::lock_guard<std::mutex> lock(mutex_);

    page_id_t new_page_id = num_pages_;
    num_pages_++;

    // Extend file with zeroed page
    char zeros[PAGE_SIZE];
    std::memset(zeros, 0, PAGE_SIZE);
    file_.seekp(static_cast<std::streamoff>(new_page_id) * PAGE_SIZE);
    file_.write(zeros, PAGE_SIZE);
    file_.flush();

    return new_page_id;
}

}  // namespace minidb
