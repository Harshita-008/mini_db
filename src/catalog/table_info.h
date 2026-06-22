#pragma once

#include "common/types.h"
#include "catalog/schema.h"
#include <string>

namespace minidb {

struct TableInfo {
    std::string table_name;
    Schema schema;
    page_id_t heap_file_page_id;  // First page of heap file
    page_id_t index_root_page_id; // Root of B+ tree index (INVALID_PAGE_ID if no index)
    uint32_t row_count;

    TableInfo() : heap_file_page_id(INVALID_PAGE_ID),
                  index_root_page_id(INVALID_PAGE_ID), row_count(0) {}

    TableInfo(const std::string& name, const Schema& sch, page_id_t heap_pid)
        : table_name(name), schema(sch), heap_file_page_id(heap_pid),
          index_root_page_id(INVALID_PAGE_ID), row_count(0) {}
};

}  // namespace minidb
