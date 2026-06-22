#include "index/index_manager.h"

namespace minidb {

IndexManager::IndexManager(BufferPool* buffer_pool) : buffer_pool_(buffer_pool) {}

Status IndexManager::CreateIndex(const std::string& table_name, const std::string& column_name) {
    std::string key = MakeKey(table_name, column_name);
    if (indexes_.find(key) != indexes_.end()) {
        return Status::DuplicateKey("Index already exists for " + key);
    }

    auto tree = std::make_unique<BPlusTree>(buffer_pool_);
    tree->Create();
    indexes_[key] = std::move(tree);

    return Status::OK();
}

BPlusTree* IndexManager::GetIndex(const std::string& table_name, const std::string& column_name) {
    std::string key = MakeKey(table_name, column_name);
    auto it = indexes_.find(key);
    if (it == indexes_.end()) return nullptr;
    return it->second.get();
}

bool IndexManager::HasIndex(const std::string& table_name, const std::string& column_name) const {
    std::string key = MakeKey(table_name, column_name);
    return indexes_.find(key) != indexes_.end();
}

Status IndexManager::DropIndex(const std::string& table_name, const std::string& column_name) {
    std::string key = MakeKey(table_name, column_name);
    auto it = indexes_.find(key);
    if (it == indexes_.end()) {
        return Status::NotFound("Index not found for " + key);
    }
    indexes_.erase(it);
    return Status::OK();
}

}  // namespace minidb
