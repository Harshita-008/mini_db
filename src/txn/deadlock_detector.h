#pragma once

#include "common/types.h"
#include "txn/lock_manager.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace minidb {

// Deadlock detector using wait-for graph with cycle detection (DFS).
class DeadlockDetector {
public:
    explicit DeadlockDetector(LockManager* lock_mgr);

    // Check for deadlocks, returns the txn_id to abort (youngest in cycle), or INVALID_TXN_ID
    txn_id_t Detect();

private:
    bool HasCycle(const std::unordered_map<txn_id_t, std::vector<txn_id_t>>& graph,
                  txn_id_t node,
                  std::unordered_set<txn_id_t>& visited,
                  std::unordered_set<txn_id_t>& in_stack,
                  std::vector<txn_id_t>& cycle);

    LockManager* lock_mgr_;
};

}  // namespace minidb
