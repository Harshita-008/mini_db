#pragma once

#include "extension/columnar_store.h"
#include "storage/heap_file.h"
#include "catalog/schema.h"
#include <memory>

namespace minidb {

// Utility to convert row-store (heap file) data to columnar format.
class ColumnConverter {
public:
    // Convert all records from a heap file into a ColumnarStore
    static std::unique_ptr<ColumnarStore> Convert(HeapFile* heap_file,
                                                   const std::string& table_name,
                                                   const Schema& schema);
};

}  // namespace minidb
