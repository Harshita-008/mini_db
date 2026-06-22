#include <gtest/gtest.h>
#include "storage/page.h"
#include <cstring>

using namespace minidb;

TEST(PageTest, InitPage) {
    Page page;
    page.Init(42);
    EXPECT_EQ(page.GetPageId(), 42);
    EXPECT_EQ(page.GetRecordCount(), 0);
    EXPECT_EQ(page.GetSlotCount(), 0);
    EXPECT_GT(page.GetFreeSpace(), 0);
}

TEST(PageTest, InsertAndGetRecord) {
    Page page;
    page.Init(1);

    const char* data = "Hello, MiniDB!";
    uint16_t len = strlen(data);
    slot_num_t slot;
    Status s = page.InsertRecord(data, len, slot);
    EXPECT_TRUE(s.ok());
    EXPECT_EQ(slot, 0);
    EXPECT_EQ(page.GetRecordCount(), 1);

    char buf[256];
    uint16_t read_len;
    s = page.GetRecord(slot, buf, read_len);
    EXPECT_TRUE(s.ok());
    EXPECT_EQ(read_len, len);
    EXPECT_EQ(std::string(buf, read_len), std::string(data, len));
}

TEST(PageTest, DeleteRecord) {
    Page page;
    page.Init(1);

    const char* data = "test record";
    slot_num_t slot;
    page.InsertRecord(data, strlen(data), slot);
    EXPECT_EQ(page.GetRecordCount(), 1);

    Status s = page.DeleteRecord(slot);
    EXPECT_TRUE(s.ok());
    EXPECT_EQ(page.GetRecordCount(), 0);

    // Getting deleted record should fail
    char buf[256];
    uint16_t len;
    s = page.GetRecord(slot, buf, len);
    EXPECT_TRUE(s.IsNotFound());
}

TEST(PageTest, MultipleRecords) {
    Page page;
    page.Init(1);

    for (int i = 0; i < 100; i++) {
        std::string data = "record_" + std::to_string(i);
        slot_num_t slot;
        Status s = page.InsertRecord(data.data(), static_cast<uint16_t>(data.size()), slot);
        EXPECT_TRUE(s.ok());
    }
    EXPECT_EQ(page.GetRecordCount(), 100);
}

TEST(PageTest, PageFull) {
    Page page;
    page.Init(1);

    // Fill the page
    std::string big_data(1000, 'X');
    int count = 0;
    while (true) {
        slot_num_t slot;
        Status s = page.InsertRecord(big_data.data(), static_cast<uint16_t>(big_data.size()), slot);
        if (!s.ok()) break;
        count++;
    }
    EXPECT_GT(count, 0);
}
