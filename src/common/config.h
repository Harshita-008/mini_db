#pragma once

#include <cstdint>
#include <cstddef>

namespace minidb {

// Page configuration
constexpr size_t PAGE_SIZE = 4096;            // 4KB pages
constexpr size_t BUFFER_POOL_SIZE = 1024;     // Number of frames in buffer pool
constexpr size_t PAGE_HEADER_SIZE = 32;       // Reserved bytes for page header

// B+ Tree configuration
constexpr int BPLUS_TREE_ORDER = 128;         // Max keys per node
constexpr int BPLUS_TREE_MIN_ORDER = BPLUS_TREE_ORDER / 2;

// Record configuration
constexpr size_t MAX_RECORD_SIZE = PAGE_SIZE - PAGE_HEADER_SIZE - 64;
constexpr size_t MAX_VARCHAR_LEN = 255;

// Transaction configuration
constexpr int MAX_CONCURRENT_TXNS = 64;

// WAL configuration
constexpr size_t WAL_BUFFER_SIZE = 4096 * 16; // 64KB WAL buffer
constexpr const char* DATA_DIR = "minidb_data";
constexpr const char* WAL_FILE = "minidb_data/wal.log";
constexpr const char* CATALOG_FILE = "minidb_data/catalog.dat";

// Columnar extension
constexpr size_t COLUMN_BATCH_SIZE = 1024;    // Rows per batch in columnar scan

}  // namespace minidb
