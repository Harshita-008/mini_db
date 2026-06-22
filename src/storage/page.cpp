#include "storage/page.h"
#include <cassert>
#include <cstring>

namespace minidb {

Page::Page() {
    std::memset(data_, 0, PAGE_SIZE);
}

void Page::Init(page_id_t page_id) {
    std::memset(data_, 0, PAGE_SIZE);
    auto* header = GetHeader();
    header->page_id = page_id;
    header->slot_count = 0;
    header->free_space_begin = sizeof(PageHeader);
    header->free_space_end = PAGE_SIZE;
    header->record_count = 0;
    header->lsn = INVALID_LSN;
    header->next_page_id = INVALID_PAGE_ID;
}

page_id_t Page::GetPageId() const {
    return GetHeader()->page_id;
}

void Page::SetPageId(page_id_t pid) {
    GetHeader()->page_id = pid;
}

lsn_t Page::GetLSN() const {
    return GetHeader()->lsn;
}

void Page::SetLSN(lsn_t lsn) {
    GetHeader()->lsn = lsn;
}

page_id_t Page::GetNextPageId() const {
    return GetHeader()->next_page_id;
}

void Page::SetNextPageId(page_id_t pid) {
    GetHeader()->next_page_id = pid;
}

Status Page::InsertRecord(const char* data, uint16_t length, slot_num_t& slot_num) {
    auto* header = GetHeader();

    // Space needed: record data + possibly a new slot entry
    uint16_t space_needed = length;

    // Check if there's a deleted slot we can reuse
    slot_num_t reuse_slot = header->slot_count;
    for (slot_num_t i = 0; i < header->slot_count; i++) {
        auto* slot = GetSlotEntry(i);
        if (slot->length == 0) {
            reuse_slot = i;
            break;
        }
    }

    if (reuse_slot == header->slot_count) {
        // Need a new slot entry
        space_needed += sizeof(SlotEntry);
    }

    // Check free space
    if (header->free_space_end - header->free_space_begin < space_needed) {
        return Status::PageFull("Not enough free space in page");
    }

    // Place record at end of free space
    header->free_space_end -= length;
    std::memcpy(data_ + header->free_space_end, data, length);

    // Update slot directory
    if (reuse_slot == header->slot_count) {
        // New slot
        slot_num = header->slot_count;
        header->slot_count++;
        header->free_space_begin += sizeof(SlotEntry);
    } else {
        slot_num = reuse_slot;
    }

    auto* slot = GetSlotEntry(slot_num);
    slot->offset = header->free_space_end;
    slot->length = length;
    header->record_count++;

    return Status::OK();
}

Status Page::DeleteRecord(slot_num_t slot_num) {
    auto* header = GetHeader();

    if (slot_num >= header->slot_count) {
        return Status::NotFound("Slot number out of range");
    }

    auto* slot = GetSlotEntry(slot_num);
    if (slot->length == 0) {
        return Status::NotFound("Record already deleted");
    }

    // Mark slot as deleted (don't reclaim space — simplifies implementation)
    slot->length = 0;
    slot->offset = 0;
    header->record_count--;

    return Status::OK();
}

Status Page::GetRecord(slot_num_t slot_num, char* data, uint16_t& length) const {
    auto* header = GetHeader();

    if (slot_num >= header->slot_count) {
        return Status::NotFound("Slot number out of range");
    }

    auto* slot = GetSlotEntry(slot_num);
    if (slot->length == 0) {
        return Status::NotFound("Record has been deleted");
    }

    length = slot->length;
    std::memcpy(data, data_ + slot->offset, length);

    return Status::OK();
}

uint16_t Page::GetRecordCount() const {
    return GetHeader()->record_count;
}

uint16_t Page::GetFreeSpace() const {
    auto* header = GetHeader();
    if (header->free_space_end <= header->free_space_begin) return 0;
    return header->free_space_end - header->free_space_begin;
}

uint16_t Page::GetSlotCount() const {
    return GetHeader()->slot_count;
}

PageHeader* Page::GetHeader() {
    return reinterpret_cast<PageHeader*>(data_);
}

const PageHeader* Page::GetHeader() const {
    return reinterpret_cast<const PageHeader*>(data_);
}

SlotEntry* Page::GetSlotEntry(slot_num_t slot_num) {
    return reinterpret_cast<SlotEntry*>(
        data_ + sizeof(PageHeader) + slot_num * sizeof(SlotEntry)
    );
}

const SlotEntry* Page::GetSlotEntry(slot_num_t slot_num) const {
    return reinterpret_cast<const SlotEntry*>(
        data_ + sizeof(PageHeader) + slot_num * sizeof(SlotEntry)
    );
}

}  // namespace minidb
