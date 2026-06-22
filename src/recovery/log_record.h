#pragma once

#include "common/types.h"
#include "common/rid.h"
#include <string>
#include <cstring>
#include <vector>

namespace minidb {

// Log record types for Write-Ahead Logging
enum class LogRecordType : uint8_t {
    BEGIN = 0,
    INSERT = 1,
    DELETE_REC = 2,
    UPDATE = 3,
    COMMIT = 4,
    ABORT = 5,
    CLR = 6,        // Compensation Log Record (for undo)
    CHECKPOINT = 7,
};

// A single WAL log record
struct LogRecord {
    lsn_t lsn;
    txn_id_t txn_id;
    lsn_t prev_lsn;         // Previous LSN for this transaction
    LogRecordType type;

    // For INSERT/DELETE/UPDATE
    std::string table_name;
    RID rid;
    std::string before_image; // For undo (DELETE, UPDATE)
    std::string after_image;  // For redo (INSERT, UPDATE)
    int32_t key;              // Primary key (for index operations)

    // For CLR
    lsn_t undo_next_lsn;     // Next LSN to undo

    LogRecord() : lsn(INVALID_LSN), txn_id(INVALID_TXN_ID),
                  prev_lsn(INVALID_LSN), type(LogRecordType::BEGIN),
                  key(0), undo_next_lsn(INVALID_LSN) {}

    // Serialize to bytes
    std::string Serialize() const {
        std::string data;
        // Header: lsn(8) + txn_id(8) + prev_lsn(8) + type(1)
        data.append(reinterpret_cast<const char*>(&lsn), sizeof(lsn));
        data.append(reinterpret_cast<const char*>(&txn_id), sizeof(txn_id));
        data.append(reinterpret_cast<const char*>(&prev_lsn), sizeof(prev_lsn));
        data.append(reinterpret_cast<const char*>(&type), sizeof(type));

        // Table name (length-prefixed)
        uint16_t name_len = static_cast<uint16_t>(table_name.size());
        data.append(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        data.append(table_name);

        // RID
        data.append(reinterpret_cast<const char*>(&rid.page_id), sizeof(rid.page_id));
        data.append(reinterpret_cast<const char*>(&rid.slot_num), sizeof(rid.slot_num));

        // Key
        data.append(reinterpret_cast<const char*>(&key), sizeof(key));

        // Before image (length-prefixed)
        uint32_t before_len = static_cast<uint32_t>(before_image.size());
        data.append(reinterpret_cast<const char*>(&before_len), sizeof(before_len));
        data.append(before_image);

        // After image (length-prefixed)
        uint32_t after_len = static_cast<uint32_t>(after_image.size());
        data.append(reinterpret_cast<const char*>(&after_len), sizeof(after_len));
        data.append(after_image);

        // Undo next LSN
        data.append(reinterpret_cast<const char*>(&undo_next_lsn), sizeof(undo_next_lsn));

        // Record length prefix (for reading back)
        uint32_t total_len = static_cast<uint32_t>(data.size());
        std::string result;
        result.append(reinterpret_cast<const char*>(&total_len), sizeof(total_len));
        result.append(data);

        return result;
    }

    // Deserialize from bytes
    static LogRecord Deserialize(const char* data, size_t len) {
        LogRecord rec;
        size_t offset = 0;

        auto read = [&](void* dst, size_t n) {
            if (offset + n <= len) {
                std::memcpy(dst, data + offset, n);
                offset += n;
            }
        };

        read(&rec.lsn, sizeof(rec.lsn));
        read(&rec.txn_id, sizeof(rec.txn_id));
        read(&rec.prev_lsn, sizeof(rec.prev_lsn));
        read(&rec.type, sizeof(rec.type));

        uint16_t name_len;
        read(&name_len, sizeof(name_len));
        if (offset + name_len <= len) {
            rec.table_name.assign(data + offset, name_len);
            offset += name_len;
        }

        read(&rec.rid.page_id, sizeof(rec.rid.page_id));
        read(&rec.rid.slot_num, sizeof(rec.rid.slot_num));
        read(&rec.key, sizeof(rec.key));

        uint32_t before_len;
        read(&before_len, sizeof(before_len));
        if (offset + before_len <= len) {
            rec.before_image.assign(data + offset, before_len);
            offset += before_len;
        }

        uint32_t after_len;
        read(&after_len, sizeof(after_len));
        if (offset + after_len <= len) {
            rec.after_image.assign(data + offset, after_len);
            offset += after_len;
        }

        read(&rec.undo_next_lsn, sizeof(rec.undo_next_lsn));

        return rec;
    }
};

}  // namespace minidb
