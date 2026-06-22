#pragma once

#include "common/types.h"
#include "common/rid.h"
#include "common/status.h"
#include "txn/transaction.h"
#include <unordered_map>
#include <list>
#include <mutex>
#include <condition_variable>

namespace minidb {

enum class LockMode {
    SHARED,
    EXCLUSIVE,
};

struct LockRequest {
    txn_id_t txn_id;
    LockMode mode;
    bool granted;

    LockRequest(txn_id_t tid, LockMode m) : txn_id(tid), mode(m), granted(false) {}
};

struct LockRequestQueue {
    std::list<LockRequest> queue;
    std::condition_variable cv;
    bool upgrading = false;
};

// Lock Manager implements Strict Two-Phase Locking (S2PL).
// Locks are acquired during GROWING phase and released only at COMMIT/ABORT.
class LockManager {
public:
    LockManager() = default;

    // Acquire a shared lock on a RID
    Status LockShared(Transaction* txn, const RID& rid);

    // Acquire an exclusive lock on a RID
    Status LockExclusive(Transaction* txn, const RID& rid);

    // Release all locks held by a transaction (called at commit/abort)
    void UnlockAll(Transaction* txn);

    // Get the wait-for relationships (for deadlock detection)
    std::unordered_map<txn_id_t, std::vector<txn_id_t>> GetWaitForGraph();

private:
    bool CanGrantLock(const LockRequestQueue& queue, const LockRequest& request);

    std::mutex mutex_;
    std::unordered_map<RID, LockRequestQueue> lock_table_;
};

}  // namespace minidb
