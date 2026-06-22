#pragma once

#include "common/types.h"
#include "common/rid.h"
#include "common/config.h"
#include <cstring>
#include <vector>

namespace minidb {

// B+ Tree nodes are stored in pages. Each node occupies one page.
// We use int32_t keys (primary keys are integers).

enum class BNodeType : uint8_t {
    INTERNAL = 0,
    LEAF = 1,
};

// Common header for both internal and leaf nodes
struct BNodeHeader {
    BNodeType type;
    uint16_t num_keys;
    page_id_t parent_page_id;
    page_id_t page_id;
};

// Maximum keys that fit in a node
// For leaf: each entry is (key: 4 bytes, RID: 6 bytes) = 10 bytes
// For internal: each entry is (key: 4 bytes, child_page_id: 4 bytes) = 8 bytes, plus one extra child
constexpr int LEAF_MAX_KEYS = (PAGE_SIZE - sizeof(BNodeHeader) - sizeof(page_id_t)) / (sizeof(int32_t) + sizeof(RID));
constexpr int INTERNAL_MAX_KEYS = (PAGE_SIZE - sizeof(BNodeHeader) - sizeof(page_id_t)) / (sizeof(int32_t) + sizeof(page_id_t));

// Leaf node layout:
// [header][key0, key1, ...][rid0, rid1, ...][next_leaf_page_id]
class LeafNode {
public:
    explicit LeafNode(char* data) : data_(data) {}

    BNodeHeader* Header() { return reinterpret_cast<BNodeHeader*>(data_); }
    const BNodeHeader* Header() const { return reinterpret_cast<const BNodeHeader*>(data_); }

    void Init(page_id_t page_id) {
        auto* h = Header();
        h->type = BNodeType::LEAF;
        h->num_keys = 0;
        h->parent_page_id = INVALID_PAGE_ID;
        h->page_id = page_id;
        SetNextLeaf(INVALID_PAGE_ID);
    }

    int32_t* Keys() { return reinterpret_cast<int32_t*>(data_ + sizeof(BNodeHeader)); }
    const int32_t* Keys() const { return reinterpret_cast<const int32_t*>(data_ + sizeof(BNodeHeader)); }

    RID* Values() {
        return reinterpret_cast<RID*>(data_ + sizeof(BNodeHeader) + LEAF_MAX_KEYS * sizeof(int32_t));
    }
    const RID* Values() const {
        return reinterpret_cast<const RID*>(data_ + sizeof(BNodeHeader) + LEAF_MAX_KEYS * sizeof(int32_t));
    }

    page_id_t GetNextLeaf() const {
        page_id_t next;
        std::memcpy(&next, data_ + PAGE_SIZE - sizeof(page_id_t), sizeof(page_id_t));
        return next;
    }

    void SetNextLeaf(page_id_t next) {
        std::memcpy(data_ + PAGE_SIZE - sizeof(page_id_t), &next, sizeof(page_id_t));
    }

    int32_t KeyAt(int index) const { return Keys()[index]; }
    RID ValueAt(int index) const { return Values()[index]; }

    void SetKeyAt(int index, int32_t key) { Keys()[index] = key; }
    void SetValueAt(int index, const RID& rid) { Values()[index] = rid; }

    // Find the position where key should be inserted (first key >= target)
    int FindKeyPosition(int32_t key) const {
        int lo = 0, hi = Header()->num_keys;
        while (lo < hi) {
            int mid = (lo + hi) / 2;
            if (Keys()[mid] < key) lo = mid + 1;
            else hi = mid;
        }
        return lo;
    }

private:
    char* data_;
};

// Internal node layout:
// [header][key0, key1, ...][child0, child1, child2, ...]
// Note: n keys, n+1 children
class InternalNode {
public:
    explicit InternalNode(char* data) : data_(data) {}

    BNodeHeader* Header() { return reinterpret_cast<BNodeHeader*>(data_); }
    const BNodeHeader* Header() const { return reinterpret_cast<const BNodeHeader*>(data_); }

    void Init(page_id_t page_id) {
        auto* h = Header();
        h->type = BNodeType::INTERNAL;
        h->num_keys = 0;
        h->parent_page_id = INVALID_PAGE_ID;
        h->page_id = page_id;
    }

    int32_t* Keys() { return reinterpret_cast<int32_t*>(data_ + sizeof(BNodeHeader)); }
    const int32_t* Keys() const { return reinterpret_cast<const int32_t*>(data_ + sizeof(BNodeHeader)); }

    page_id_t* Children() {
        return reinterpret_cast<page_id_t*>(data_ + sizeof(BNodeHeader) + INTERNAL_MAX_KEYS * sizeof(int32_t));
    }
    const page_id_t* Children() const {
        return reinterpret_cast<const page_id_t*>(data_ + sizeof(BNodeHeader) + INTERNAL_MAX_KEYS * sizeof(int32_t));
    }

    int32_t KeyAt(int index) const { return Keys()[index]; }
    page_id_t ChildAt(int index) const { return Children()[index]; }

    void SetKeyAt(int index, int32_t key) { Keys()[index] = key; }
    void SetChildAt(int index, page_id_t child) { Children()[index] = child; }

    // Find the child index for a given key
    int FindChildIndex(int32_t key) const {
        int lo = 0, hi = Header()->num_keys;
        while (lo < hi) {
            int mid = (lo + hi) / 2;
            if (Keys()[mid] <= key) lo = mid + 1;
            else hi = mid;
        }
        return lo;
    }

private:
    char* data_;
};

}  // namespace minidb
