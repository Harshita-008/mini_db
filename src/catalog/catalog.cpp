#include "catalog/catalog.h"

namespace minidb {

Catalog::Catalog(BufferPool* buffer_pool) : buffer_pool_(buffer_pool) {}

Status Catalog::CreateTable(const std::string& name, const Schema& schema) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (tables_.find(name) != tables_.end()) {
        return Status::DuplicateKey("Table '" + name + "' already exists");
    }

    // Create a heap file for the table
    HeapFile heap = HeapFile::Create(buffer_pool_);
    page_id_t heap_page_id = heap.GetFirstPageId();

    // Store table info
    TableInfo info(name, schema, heap_page_id);
    tables_[name] = info;
    heap_files_[name] = std::make_unique<HeapFile>(buffer_pool_, heap_page_id);

    return Status::OK();
}

Status Catalog::DropTable(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (tables_.find(name) == tables_.end()) {
        return Status::NotFound("Table '" + name + "' not found");
    }

    tables_.erase(name);
    heap_files_.erase(name);

    return Status::OK();
}

TableInfo* Catalog::GetTable(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = tables_.find(name);
    if (it == tables_.end()) return nullptr;
    return &it->second;
}

HeapFile* Catalog::GetHeapFile(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = heap_files_.find(name);
    if (it == heap_files_.end()) return nullptr;
    return it->second.get();
}

bool Catalog::TableExists(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tables_.find(name) != tables_.end();
}

std::vector<std::string> Catalog::GetTableNames() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    for (const auto& [name, _] : tables_) {
        names.push_back(name);
    }
    return names;
}

void Catalog::IncrementRowCount(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tables_.find(name);
    if (it != tables_.end()) {
        it->second.row_count++;
    }
}

void Catalog::DecrementRowCount(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tables_.find(name);
    if (it != tables_.end() && it->second.row_count > 0) {
        it->second.row_count--;
    }
}

}  // namespace minidb
