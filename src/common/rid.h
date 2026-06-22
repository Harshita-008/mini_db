#pragma once

#include "common/types.h"
#include <functional>

namespace minidb {

// Record ID — uniquely identifies a record on disk
struct RID {
    page_id_t page_id;
    slot_num_t slot_num;

    RID() : page_id(INVALID_PAGE_ID), slot_num(0) {}
    RID(page_id_t pid, slot_num_t snum) : page_id(pid), slot_num(snum) {}

    bool operator==(const RID& other) const {
        return page_id == other.page_id && slot_num == other.slot_num;
    }

    bool operator!=(const RID& other) const {
        return !(*this == other);
    }

    bool operator<(const RID& other) const {
        if (page_id != other.page_id) return page_id < other.page_id;
        return slot_num < other.slot_num;
    }

    bool IsValid() const {
        return page_id != INVALID_PAGE_ID;
    }
};

}  // namespace minidb

// Hash support for RID
namespace std {
template<>
struct hash<minidb::RID> {
    size_t operator()(const minidb::RID& rid) const {
        return hash<uint64_t>()(
            (static_cast<uint64_t>(rid.page_id) << 16) | rid.slot_num
        );
    }
};
}  // namespace std
