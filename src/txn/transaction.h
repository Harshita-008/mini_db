#pragma once

#include "common/types.h"
#include "common/rid.h"
#include <vector>
#include <unordered_set>
#include <string>

namespace minidb {

enum class TxnState {
    GROWING,     // Acquiring locks (before first unlock)
    SHRINKING,   // Releasing locks (no new locks allowed)
    COMMITTED,
    ABORTED,
};

// Undo action for rollback
struct UndoAction {
    enum Type { INSERT, DELETE_ACT } type;
    std::string table_name;
    RID rid;
    std::string before_image;  // For DELETE undo (re-insert)
    uint16_t data_length;
    int32_t key;               // Primary key for index undo
};

class Transaction {
public:
    explicit Transaction(txn_id_t txn_id);

    txn_id_t GetTxnId() const { return txn_id_; }
    TxnState GetState() const { return state_; }
    void SetState(TxnState state) { state_ = state; }

    // Lock management
    void AddSharedLock(const RID& rid) { shared_locks_.insert(rid); }
    void AddExclusiveLock(const RID& rid) { exclusive_locks_.insert(rid); }
    const std::unordered_set<RID>& GetSharedLocks() const { return shared_locks_; }
    const std::unordered_set<RID>& GetExclusiveLocks() const { return exclusive_locks_; }

    // Undo log for rollback
    void AddUndoAction(const UndoAction& action) { undo_log_.push_back(action); }
    const std::vector<UndoAction>& GetUndoLog() const { return undo_log_; }

    lsn_t GetPrevLSN() const { return prev_lsn_; }
    void SetPrevLSN(lsn_t lsn) { prev_lsn_ = lsn; }

private:
    txn_id_t txn_id_;
    TxnState state_;
    std::unordered_set<RID> shared_locks_;
    std::unordered_set<RID> exclusive_locks_;
    std::vector<UndoAction> undo_log_;
    lsn_t prev_lsn_;
};

}  // namespace minidb
