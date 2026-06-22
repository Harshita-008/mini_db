#include "recovery/recovery_manager.h"
#include <iostream>
#include <algorithm>

namespace minidb {

RecoveryManager::RecoveryManager(LogManager* log_mgr, BufferPool* buffer_pool, Catalog* catalog)
    : log_mgr_(log_mgr), buffer_pool_(buffer_pool), catalog_(catalog) {}

lsn_t RecoveryManager::LogBegin(txn_id_t txn_id) {
    LogRecord rec;
    rec.txn_id = txn_id;
    rec.type = LogRecordType::BEGIN;
    rec.prev_lsn = txn_prev_lsn_[txn_id];
    lsn_t lsn = log_mgr_->AppendLog(rec);
    txn_prev_lsn_[txn_id] = lsn;
    return lsn;
}

lsn_t RecoveryManager::LogInsert(txn_id_t txn_id, const std::string& table,
                                  const RID& rid, const std::string& data, int32_t key) {
    LogRecord rec;
    rec.txn_id = txn_id;
    rec.type = LogRecordType::INSERT;
    rec.prev_lsn = txn_prev_lsn_[txn_id];
    rec.table_name = table;
    rec.rid = rid;
    rec.after_image = data;
    rec.key = key;
    lsn_t lsn = log_mgr_->AppendLog(rec);
    txn_prev_lsn_[txn_id] = lsn;
    return lsn;
}

lsn_t RecoveryManager::LogDelete(txn_id_t txn_id, const std::string& table,
                                  const RID& rid, const std::string& before_image, int32_t key) {
    LogRecord rec;
    rec.txn_id = txn_id;
    rec.type = LogRecordType::DELETE_REC;
    rec.prev_lsn = txn_prev_lsn_[txn_id];
    rec.table_name = table;
    rec.rid = rid;
    rec.before_image = before_image;
    rec.key = key;
    lsn_t lsn = log_mgr_->AppendLog(rec);
    txn_prev_lsn_[txn_id] = lsn;
    return lsn;
}

lsn_t RecoveryManager::LogCommit(txn_id_t txn_id) {
    LogRecord rec;
    rec.txn_id = txn_id;
    rec.type = LogRecordType::COMMIT;
    rec.prev_lsn = txn_prev_lsn_[txn_id];
    lsn_t lsn = log_mgr_->AppendLog(rec);
    log_mgr_->Flush(lsn);  // Force flush on commit
    txn_prev_lsn_.erase(txn_id);
    return lsn;
}

lsn_t RecoveryManager::LogAbort(txn_id_t txn_id) {
    LogRecord rec;
    rec.txn_id = txn_id;
    rec.type = LogRecordType::ABORT;
    rec.prev_lsn = txn_prev_lsn_[txn_id];
    lsn_t lsn = log_mgr_->AppendLog(rec);
    log_mgr_->Flush(lsn);
    txn_prev_lsn_.erase(txn_id);
    return lsn;
}

bool RecoveryManager::NeedsRecovery() {
    auto logs = log_mgr_->ReadAllLogs();
    if (logs.empty()) return false;

    // Check if there are uncommitted transactions
    std::unordered_set<txn_id_t> active;
    for (const auto& rec : logs) {
        if (rec.type == LogRecordType::BEGIN) active.insert(rec.txn_id);
        else if (rec.type == LogRecordType::COMMIT || rec.type == LogRecordType::ABORT)
            active.erase(rec.txn_id);
    }
    return !active.empty();
}

void RecoveryManager::Recover() {
    auto logs = log_mgr_->ReadAllLogs();
    if (logs.empty()) return;

    std::cout << "[Recovery] Starting ARIES recovery with " << logs.size() << " log records\n";

    // Phase 1: Analysis
    std::unordered_set<txn_id_t> active_txns;
    std::unordered_map<txn_id_t, lsn_t> last_lsn;
    AnalysisPhase(logs, active_txns, last_lsn);
    std::cout << "[Recovery] Analysis: " << active_txns.size() << " active transactions found\n";

    // Phase 2: Redo
    RedoPhase(logs);
    std::cout << "[Recovery] Redo phase complete\n";

    // Phase 3: Undo
    UndoPhase(logs, active_txns, last_lsn);
    std::cout << "[Recovery] Undo phase complete\n";

    // Clear WAL after successful recovery
    log_mgr_->Clear();
    std::cout << "[Recovery] Recovery complete, WAL cleared\n";
}

void RecoveryManager::AnalysisPhase(const std::vector<LogRecord>& logs,
                                     std::unordered_set<txn_id_t>& active_txns,
                                     std::unordered_map<txn_id_t, lsn_t>& last_lsn) {
    for (const auto& rec : logs) {
        last_lsn[rec.txn_id] = rec.lsn;

        switch (rec.type) {
            case LogRecordType::BEGIN:
                active_txns.insert(rec.txn_id);
                break;
            case LogRecordType::COMMIT:
            case LogRecordType::ABORT:
                active_txns.erase(rec.txn_id);
                break;
            default:
                break;
        }
    }
}

void RecoveryManager::RedoPhase(const std::vector<LogRecord>& logs) {
    for (const auto& rec : logs) {
        if (rec.type == LogRecordType::INSERT) {
            // Redo the insert
            HeapFile* heap = catalog_->GetHeapFile(rec.table_name);
            if (heap) {
                // Check if record already exists (idempotent redo)
                char existing[4096];
                uint16_t len;
                Status s = heap->GetRecord(rec.rid, existing, len);
                if (!s.ok()) {
                    // Record doesn't exist — redo the insert
                    RID new_rid;
                    heap->InsertRecord(rec.after_image.data(),
                                      static_cast<uint16_t>(rec.after_image.size()), new_rid);
                }
            }
        } else if (rec.type == LogRecordType::DELETE_REC) {
            HeapFile* heap = catalog_->GetHeapFile(rec.table_name);
            if (heap) {
                heap->DeleteRecord(rec.rid);
            }
        }
    }
}

void RecoveryManager::UndoPhase(const std::vector<LogRecord>& logs,
                                 const std::unordered_set<txn_id_t>& active_txns,
                                 const std::unordered_map<txn_id_t, lsn_t>& last_lsn) {
    if (active_txns.empty()) return;

    // Process logs in reverse order
    for (auto it = logs.rbegin(); it != logs.rend(); ++it) {
        const auto& rec = *it;

        if (active_txns.find(rec.txn_id) == active_txns.end()) continue;

        if (rec.type == LogRecordType::INSERT) {
            // Undo insert = delete
            HeapFile* heap = catalog_->GetHeapFile(rec.table_name);
            if (heap) {
                heap->DeleteRecord(rec.rid);
            }
        } else if (rec.type == LogRecordType::DELETE_REC) {
            // Undo delete = re-insert
            HeapFile* heap = catalog_->GetHeapFile(rec.table_name);
            if (heap) {
                RID new_rid;
                heap->InsertRecord(rec.before_image.data(),
                                  static_cast<uint16_t>(rec.before_image.size()), new_rid);
            }
        }
    }
}

}  // namespace minidb
