#include "txn/deadlock_detector.h"
#include <algorithm>

namespace minidb {

DeadlockDetector::DeadlockDetector(LockManager* lock_mgr) : lock_mgr_(lock_mgr) {}

txn_id_t DeadlockDetector::Detect() {
    auto graph = lock_mgr_->GetWaitForGraph();
    if (graph.empty()) return INVALID_TXN_ID;

    std::unordered_set<txn_id_t> visited;
    std::unordered_set<txn_id_t> in_stack;
    std::vector<txn_id_t> cycle;

    for (const auto& [node, _] : graph) {
        if (visited.find(node) == visited.end()) {
            if (HasCycle(graph, node, visited, in_stack, cycle)) {
                // Abort the youngest transaction in the cycle (highest txn_id)
                return *std::max_element(cycle.begin(), cycle.end());
            }
        }
    }

    return INVALID_TXN_ID;
}

bool DeadlockDetector::HasCycle(
    const std::unordered_map<txn_id_t, std::vector<txn_id_t>>& graph,
    txn_id_t node,
    std::unordered_set<txn_id_t>& visited,
    std::unordered_set<txn_id_t>& in_stack,
    std::vector<txn_id_t>& cycle) {

    visited.insert(node);
    in_stack.insert(node);
    cycle.push_back(node);

    auto it = graph.find(node);
    if (it != graph.end()) {
        for (txn_id_t neighbor : it->second) {
            if (in_stack.find(neighbor) != in_stack.end()) {
                cycle.push_back(neighbor);
                return true;  // Found cycle
            }
            if (visited.find(neighbor) == visited.end()) {
                if (HasCycle(graph, neighbor, visited, in_stack, cycle)) {
                    return true;
                }
            }
        }
    }

    in_stack.erase(node);
    cycle.pop_back();
    return false;
}

}  // namespace minidb
