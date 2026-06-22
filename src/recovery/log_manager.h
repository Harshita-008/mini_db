#pragma once

#include "common/types.h"
#include "common/status.h"
#include "recovery/log_record.h"
#include <string>
#include <vector>
#include <mutex>
#include <fstream>
#include <atomic>

namespace minidb {

// Log Manager — appends log records to the WAL file sequentially.
class LogManager {
public:
    explicit LogManager(const std::string& log_file_path);
    ~LogManager();

    // Append a log record, returns the assigned LSN
    lsn_t AppendLog(LogRecord& record);

    // Flush WAL up to the given LSN
    void Flush(lsn_t up_to_lsn = INVALID_LSN);

    // Read all log records from the WAL file
    std::vector<LogRecord> ReadAllLogs();

    // Get the current (next) LSN
    lsn_t GetNextLSN() const { return next_lsn_.load(); }

    // Clear the WAL file (after successful recovery)
    void Clear();

private:
    std::string log_file_path_;
    std::ofstream log_file_;
    std::atomic<lsn_t> next_lsn_{1};
    lsn_t flushed_lsn_ = 0;
    std::vector<std::string> buffer_;  // Buffered log records
    std::mutex mutex_;
};

}  // namespace minidb
