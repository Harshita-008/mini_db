#include <gtest/gtest.h>
#include "storage/buffer_pool.h"
#include "storage/page_manager.h"
#include <filesystem>

using namespace minidb;

class BufferPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::filesystem::create_directories("test_data");
        pm = std::make_unique<PageManager>("test_data/test_bp.db");
        bp = std::make_unique<BufferPool>(pm.get(), 10);
    }
    void TearDown() override {
        bp.reset(); pm.reset();
        std::filesystem::remove_all("test_data");
    }
    std::unique_ptr<PageManager> pm;
    std::unique_ptr<BufferPool> bp;
};

TEST_F(BufferPoolTest, NewPage) {
    page_id_t pid;
    Page* page = bp->NewPage(pid);
    ASSERT_NE(page, nullptr);
    EXPECT_EQ(page->GetPageId(), pid);
    bp->UnpinPage(pid, true);
}

TEST_F(BufferPoolTest, FetchPage) {
    page_id_t pid;
    Page* page = bp->NewPage(pid);
    ASSERT_NE(page, nullptr);

    const char* data = "test data";
    slot_num_t slot;
    page->InsertRecord(data, strlen(data), slot);
    bp->UnpinPage(pid, true);
    bp->FlushPage(pid);

    Page* fetched = bp->FetchPage(pid);
    ASSERT_NE(fetched, nullptr);
    char buf[256]; uint16_t len;
    EXPECT_TRUE(fetched->GetRecord(slot, buf, len).ok());
    EXPECT_EQ(std::string(buf, len), "test data");
    bp->UnpinPage(pid, false);
}

TEST_F(BufferPoolTest, Eviction) {
    // Create more pages than pool size (10)
    std::vector<page_id_t> pages;
    for (int i = 0; i < 10; i++) {
        page_id_t pid;
        Page* p = bp->NewPage(pid);
        ASSERT_NE(p, nullptr);
        pages.push_back(pid);
        bp->UnpinPage(pid, true);
    }
    // Should be able to create more (eviction kicks in)
    page_id_t pid;
    Page* p = bp->NewPage(pid);
    ASSERT_NE(p, nullptr);
    bp->UnpinPage(pid, true);
}
