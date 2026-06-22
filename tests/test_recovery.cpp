#include <gtest/gtest.h>
#include "recovery/log_manager.h"
#include "recovery/recovery_manager.h"
#include "storage/page_manager.h"
#include "storage/buffer_pool.h"
#include "catalog/catalog.h"
#include <filesystem>

using namespace minidb;

class RecoveryTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::filesystem::create_directories("test_data");
        pm = std::make_unique<PageManager>("test_data/test_recovery.db");
        bp = std::make_unique<BufferPool>(pm.get(), 64);
        catalog = std::make_unique<Catalog>(bp.get());
        log_mgr = std::make_unique<LogManager>("test_data/test_wal.log");
        recovery_mgr = std::make_unique<RecoveryManager>(log_mgr.get(), bp.get(), catalog.get());
    }
    void TearDown() override {
        recovery_mgr.reset(); log_mgr.reset(); catalog.reset(); bp.reset(); pm.reset();
        std::filesystem::remove_all("test_data");
    }
    std::unique_ptr<PageManager> pm;
    std::unique_ptr<BufferPool> bp;
    std::unique_ptr<Catalog> catalog;
    std::unique_ptr<LogManager> log_mgr;
    std::unique_ptr<RecoveryManager> recovery_mgr;
};

TEST_F(RecoveryTest, LogBeginCommit) {
    lsn_t lsn1 = recovery_mgr->LogBegin(1);
    EXPECT_GT(lsn1, 0UL);

    lsn_t lsn2 = recovery_mgr->LogCommit(1);
    EXPECT_GT(lsn2, lsn1);
}

TEST_F(RecoveryTest, LogInsertDelete) {
    recovery_mgr->LogBegin(1);
    lsn_t lsn = recovery_mgr->LogInsert(1, "users", RID(0, 0), "test_data", 42);
    EXPECT_GT(lsn, 0UL);

    lsn = recovery_mgr->LogDelete(1, "users", RID(0, 0), "test_data", 42);
    EXPECT_GT(lsn, 0UL);

    recovery_mgr->LogCommit(1);
}

TEST_F(RecoveryTest, ReadWriteLogs) {
    recovery_mgr->LogBegin(1);
    recovery_mgr->LogInsert(1, "test", RID(1, 0), "record1", 1);
    recovery_mgr->LogCommit(1);
    log_mgr->Flush();

    auto logs = log_mgr->ReadAllLogs();
    EXPECT_EQ(logs.size(), 3); // BEGIN, INSERT, COMMIT
    EXPECT_EQ(logs[0].type, LogRecordType::BEGIN);
    EXPECT_EQ(logs[1].type, LogRecordType::INSERT);
    EXPECT_EQ(logs[2].type, LogRecordType::COMMIT);
}

TEST_F(RecoveryTest, NeedsRecoveryCommitted) {
    recovery_mgr->LogBegin(1);
    recovery_mgr->LogCommit(1);
    log_mgr->Flush();

    EXPECT_FALSE(recovery_mgr->NeedsRecovery());
}

TEST_F(RecoveryTest, NeedsRecoveryUncommitted) {
    recovery_mgr->LogBegin(1);
    recovery_mgr->LogInsert(1, "test", RID(0, 0), "data", 1);
    log_mgr->Flush();
    // No commit — simulates crash

    EXPECT_TRUE(recovery_mgr->NeedsRecovery());
}

TEST_F(RecoveryTest, ClearWAL) {
    recovery_mgr->LogBegin(1);
    recovery_mgr->LogCommit(1);
    log_mgr->Flush();

    log_mgr->Clear();
    auto logs = log_mgr->ReadAllLogs();
    EXPECT_EQ(logs.size(), 0);
}
