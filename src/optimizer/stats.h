#pragma once

#include "common/types.h"
#include <string>
#include <unordered_map>

namespace minidb {

// Per-column statistics
struct ColumnStats {
    int64_t distinct_values = 0;
    int32_t min_value = 0;
    int32_t max_value = 0;
};

// Per-table statistics for cost estimation
struct TableStats {
    uint32_t row_count = 0;
    uint32_t page_count = 0;
    std::unordered_map<std::string, ColumnStats> column_stats;

    void UpdateOnInsert(int32_t pk_value) {
        row_count++;
        auto& pk = column_stats["_pk"];
        pk.distinct_values++;
        if (pk_value < pk.min_value || pk.distinct_values == 1) pk.min_value = pk_value;
        if (pk_value > pk.max_value || pk.distinct_values == 1) pk.max_value = pk_value;
    }

    void UpdateOnDelete() {
        if (row_count > 0) row_count--;
    }
};

// Global statistics store
class StatsManager {
public:
    TableStats& GetStats(const std::string& table_name) {
        return stats_[table_name];
    }

    void SetStats(const std::string& table_name, const TableStats& stats) {
        stats_[table_name] = stats;
    }

    bool HasStats(const std::string& table_name) const {
        return stats_.find(table_name) != stats_.end();
    }

private:
    std::unordered_map<std::string, TableStats> stats_;
};

}  // namespace minidb
