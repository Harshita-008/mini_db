#pragma once

#include "common/config.h"
#include "common/types.h"
#include "common/rid.h"
#include "common/status.h"
#include <cstring>
#include <vector>
#include <string>

namespace minidb {

// Slotted page layout:
// +------------------+
// | PageHeader       |  (32 bytes)
// +------------------+
// | Slot Directory   |  (grows downward: slot_count * 4 bytes)
// |   slot[0]        |
// |   slot[1]        |
// |   ...            |
// +------------------+
// |   Free Space     |
// +------------------+
// |   ...            |
// |   Record Data    |  (grows upward from end of page)
// |   record[1]      |
// |   record[0]      |
// +------------------+

struct PageHeader {
    page_id_t page_id;
    uint16_t slot_count;        // Number of slots in directory
    uint16_t free_space_begin;  // Start of free space (after slot directory)
    uint16_t free_space_end;    // End of free space (before record data)
    uint16_t record_count;      // Number of live records
    lsn_t lsn;                  // Log sequence number for recovery
    page_id_t next_page_id;     // Linked list pointer for heap file
};

struct SlotEntry {
    uint16_t offset;  // Offset of record from start of page
    uint16_t length;  // Length of record (0 = deleted/empty)
};

class Page {
public:
    Page();

    void Init(page_id_t page_id);
    page_id_t GetPageId() const;
    void SetPageId(page_id_t pid);

    lsn_t GetLSN() const;
    void SetLSN(lsn_t lsn);

    page_id_t GetNextPageId() const;
    void SetNextPageId(page_id_t pid);

    // Record operations
    // Returns slot_num on success, or Status error
    Status InsertRecord(const char* data, uint16_t length, slot_num_t& slot_num);
    Status DeleteRecord(slot_num_t slot_num);
    Status GetRecord(slot_num_t slot_num, char* data, uint16_t& length) const;

    uint16_t GetRecordCount() const;
    uint16_t GetFreeSpace() const;
    uint16_t GetSlotCount() const;

    // Direct access to raw data
    char* GetData() { return data_; }
    const char* GetData() const { return data_; }

private:
    PageHeader* GetHeader();
    const PageHeader* GetHeader() const;
    SlotEntry* GetSlotEntry(slot_num_t slot_num);
    const SlotEntry* GetSlotEntry(slot_num_t slot_num) const;

    char data_[PAGE_SIZE];
};

}  // namespace minidb
