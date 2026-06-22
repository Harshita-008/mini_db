#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>
#include <optional>

namespace minidb {

// Fundamental ID types
using page_id_t = uint32_t;
using frame_id_t = uint32_t;
using txn_id_t = uint64_t;
using lsn_t = uint64_t;
using slot_num_t = uint16_t;
using table_id_t = uint32_t;
using index_id_t = uint32_t;
using col_id_t = uint16_t;

// Invalid sentinels
constexpr page_id_t INVALID_PAGE_ID = 0xFFFFFFFF;
constexpr frame_id_t INVALID_FRAME_ID = 0xFFFFFFFF;
constexpr txn_id_t INVALID_TXN_ID = 0;
constexpr lsn_t INVALID_LSN = 0;
constexpr table_id_t INVALID_TABLE_ID = 0xFFFFFFFF;

// Column data types
enum class ColumnType : uint8_t {
    INT = 0,
    FLOAT = 1,
    VARCHAR = 2,
    BOOL = 3,
};

// Value type — a single database value
using Value = std::variant<int32_t, double, std::string, bool>;

// Tuple — a row of values
using Tuple = std::vector<Value>;

// String representation helper
inline std::string column_type_to_string(ColumnType t) {
    switch (t) {
        case ColumnType::INT: return "INT";
        case ColumnType::FLOAT: return "FLOAT";
        case ColumnType::VARCHAR: return "VARCHAR";
        case ColumnType::BOOL: return "BOOL";
        default: return "UNKNOWN";
    }
}

inline size_t column_type_size(ColumnType t) {
    switch (t) {
        case ColumnType::INT: return sizeof(int32_t);
        case ColumnType::FLOAT: return sizeof(double);
        case ColumnType::BOOL: return sizeof(bool);
        case ColumnType::VARCHAR: return 0; // variable
        default: return 0;
    }
}

}  // namespace minidb
