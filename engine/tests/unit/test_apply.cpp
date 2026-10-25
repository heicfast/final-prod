/* test_apply.cpp - the consent-gated lost-partition apply suite
 * (S-029, diff-to-fix: the og's two-step product flow - discover, then
 * APPLY the recovered entry into the user's MBR - through the rawfixer
 * safety seams: plan/execute separation, consent callback, revalidation,
 * transactional writes, CRUNDO01 undo).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include <gtest/gtest.h>
#include "cr/apply/cr_apply.h"
#include "cr/repair/cr_rawfixer.h"

#include <cstring>
#include <vector>

using namespace cr;
using namespace cr::apply;
using lost::CandidateFs;

namespace {

constexpr u64 kSec = 512;
constexpr u64 kDiskSecs = 16384; /* 8 MiB test disk */

class MemDisk final : public IStorageDevice {
public:
    explicit MemDisk(u64 sectors = kDiskSecs) : data_(size_t(sectors) * kSec, 0) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = sectors;
        geom_.total_bytes = data_.size();
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }
    std::vector<u8>& raw() { return data_; }

private:
    std::vector<u8> data_;
    Geometry geom_;
};

/* a huge disk without the memory cost: only sector 0 is real (the
 * planner reads nothing else); the size/geometry answer is what the
 * containment checks consume. */
class HugeDisk final : public IStorageDevice {
public:
    explicit HugeDisk(u64 sectors) : sectors_(sectors), sec0_(kSec, 0) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = sectors;
        geom_.total_bytes = sectors * kSec;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off >= kSec) { /* beyond the one real sector: zeros */
            std::memset(buf, 0, len);
            return OK;
        }
        const u32 n = off + len <= kSec ? len : u32(kSec - off);
        std::memcpy(buf, sec0_.data() + off, n);
        if (n < len)
            std::memset(static_cast<u8*>(buf) + n, 0, len - n);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }
    std::vector<u8>& sector0() { return sec0_; }

private:
    u64 sectors_;
    std::vector<u8> sec0_;
    Geometry geom_;
};

/* write-capable RAM device: records the call order (lock before write -
 * the quiesce contract) and can be told to fail the Nth write. */
class WriteMem final : public repair::IWriteDevice {
public:
    explicit WriteMem(std::vector<u8> backing)
        : data_(std::move(backing)), orig_(data_) {}
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    u32 write_at(u64 off, const void* buf, u32 len) override {
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (locks_ == 0)
            lock_order_violation_ = true; /* a write before lock_volume */
        if (++writes_ == fail_nth_write_)
            return ERR_WRITE_FAILED;
        std::memcpy(data_.data() + off, buf, len);
        ++committed_;
        return OK;
    }
    u64 size() const override { return data_.size(); }
    u32 sector_bytes() const override { return 512; }
    u32 lock_volume() override {
        ++locks_;
        return lock_rc_;
    }
    const std::vector<u8>& raw() const { return data_; }
    std::vector<u8>& mutable_raw() { return data_; } /* test-side tampering */
    const std::vector<u8>& original() const { return orig_; }
    bool lock_order_violation_ = false;
    u32 lock_rc_ = OK;
    u32 fail_nth_write_ = 0; /* 1-based; 0 = never fail */
    u32 writes_ = 0;
    u32 locks_ = 0;
    u32 committed_ = 0;

private:
    std::vector<u8> data_;
    std::vector<u8> orig_;
};

/* an MBR with one live FAT partition at LBA 2048 (2048 sectors) and three
 * free slots */
std::vector<u8> mbr_disk() {
    MemDisk d;
    d.raw()[510] = 0x55;
    d.raw()[511] = 0xAA;
    u8* e = d.raw().data() + 446;
    e[0] = 0x00;
    e[1] = 0x20; /* CHS 0,32,33 */
    e[2] = 0x21;
    e[3] = 0x00;
    e[4] = 0x06; /* FAT16 */
    e[5] = 0xFE;
    e[6] = 0xFF;
    e[7] = 0xFF;
    u32 lba = 2048, cnt = 1024;
    std::memcpy(e + 8, &lba, 4);
    std::memcpy(e + 12, &cnt, 4);
    return d.raw();
}

lost::Candidate cand(u64 lba, u64 count, CandidateFs fs) {
    lost::Candidate c;
    c.start_lba = lba;
    c.sector_count = count;
    c.fs = fs;
    c.score = 60;
    c.evidence.push_back({"boot-sig", 30});
    return c;
}

bool allow(const ApplyPlan&, void*) { return true; }
bool deny(const ApplyPlan&, void*) { return false; }

u32 rd32(const u8* p) {
    return u32(p[0]) | u32(p[1]) << 8 | u32(p[2]) << 16 | u32(p[3]) << 24;
}

} // namespace

TEST(Apply, PlansEntryIntoFirstFreeSlot) {
    MemDisk disk;
    disk.raw() = mbr_disk();
    ApplyPlan plan;
    ASSERT_EQ(plan_apply(&disk, cand(4096, 512, CandidateFs::Ntfs), plan), OK);
    ASSERT_FALSE(plan.refused) << plan.refusal_reason;
    EXPECT_EQ(plan.entry_index, 1u); /* slot 0 is live, slot 1 is first free */
    EXPECT_EQ(plan.mbr_type, 0x07);
    ASSERT_EQ(plan.entry_bytes.size(), 16u);
    const u8* e = plan.entry_bytes.data();
    EXPECT_EQ(e[0], 0x00); /* not bootable */
    EXPECT_EQ(e[4], 0x07);
    EXPECT_EQ(rd32(e + 8), 4096u);
    EXPECT_EQ(rd32(e + 12), 512u);
    ASSERT_EQ(plan.actions.size(), 1u); /* signature intact: no restore */
    EXPECT_EQ(plan.actions[0].offset, 446u + 16u); /* slot 1's offset */
    EXPECT_EQ(plan.sector0_original.size(), 512u);
    EXPECT_FALSE(plan.boot_repair_available); /* M-040 truthfully absent */
}

TEST(Apply, ExecuteWritesEntryVerifiesAndUndoes) {
    ApplyPlan plan;
    {
        MemDisk disk;
        disk.raw() = mbr_disk();
        ASSERT_EQ(plan_apply(&disk, cand(4096, 512, CandidateFs::Ntfs), plan), OK);
        ASSERT_FALSE(plan.refused);
    }
    WriteMem dev(mbr_disk());
    repair::UndoLog undo;
    ASSERT_EQ(execute_apply(&dev, plan, allow, nullptr, undo), OK);
    EXPECT_EQ(dev.locks_, 1u) << "the quiesce ran exactly once";
    EXPECT_FALSE(dev.lock_order_violation_) << "lock_volume precedes every write";
    EXPECT_EQ(dev.committed_, 1u);

    /* the entry landed in slot 1 with the planned bytes; slot 0 + the
     * signature are untouched */
    const u8* e = dev.raw().data() + 446 + 16;
    EXPECT_EQ(e[4], 0x07);
    EXPECT_EQ(rd32(e + 8), 4096u);
    EXPECT_EQ(rd32(e + 12), 512u);
    EXPECT_EQ(dev.raw()[446 + 8], mbr_disk()[446 + 8]) << "slot 0 untouched";
    EXPECT_EQ(dev.raw()[510], 0x55);
    EXPECT_EQ(dev.raw()[511], 0xAA);
    ASSERT_EQ(undo.entries.size(), 1u);

    /* undo restores the exact original sector 0 (the CRUNDO01 container
     * 'fix --undo-apply' replays) */
    ASSERT_EQ(repair::undo(dev, undo), OK);
    EXPECT_EQ(dev.raw(), dev.original()) << "undo restores sector 0 byte-exact";
}

TEST(Apply, RefusedWithoutConsent) {
    ApplyPlan plan;
    {
        MemDisk disk;
        disk.raw() = mbr_disk();
        ASSERT_EQ(plan_apply(&disk, cand(4096, 512, CandidateFs::Ntfs), plan), OK);
        ASSERT_FALSE(plan.refused);
    }
    WriteMem dev(mbr_disk());
    repair::UndoLog undo;
    /* A1: a DENIED callback writes nothing */
    EXPECT_EQ(execute_apply(&dev, plan, deny, nullptr, undo), ERR_APPLY_CONSENT_DENIED);
    EXPECT_EQ(dev.committed_, 0u) << "consent denial performs ZERO writes";
    EXPECT_TRUE(undo.entries.empty());
    EXPECT_EQ(dev.raw(), dev.original());
    /* A1: an ABSENT callback is a denial, not an implicit yes */
    EXPECT_EQ(execute_apply(&dev, plan, nullptr, nullptr, undo), ERR_APPLY_CONSENT_DENIED);
    EXPECT_EQ(dev.committed_, 0u);
}

TEST(Apply, StaleTableRefusedBeforeConsent) {
    ApplyPlan plan;
    {
        MemDisk disk;
        disk.raw() = mbr_disk();
        ASSERT_EQ(plan_apply(&disk, cand(4096, 512, CandidateFs::Ntfs), plan), OK);
    }
    WriteMem dev(mbr_disk());
    /* the table changed after planning (a third-party tool wrote slot 3) */
    u8 other[16] = {};
    other[4] = 0x07;
    u32 lba = 3000, cnt = 100;
    std::memcpy(other + 8, &lba, 4);
    std::memcpy(other + 12, &cnt, 4);
    std::memcpy(dev.mutable_raw().data() + 446 + 3 * 16, other, 16);
    repair::UndoLog undo;
    /* A2: the mismatch is a stale-plan refusal - the consent callback
     * must NOT even run (revalidation precedes consent) */
    EXPECT_EQ(execute_apply(&dev, plan, allow, nullptr, undo), ERR_APPLY_STALE);
    EXPECT_EQ(dev.committed_, 0u);
    EXPECT_TRUE(undo.entries.empty());
}

TEST(Apply, QuiesceFailureIsACleanRefusal) {
    ApplyPlan plan;
    {
        MemDisk disk;
        disk.raw() = mbr_disk();
        ASSERT_EQ(plan_apply(&disk, cand(4096, 512, CandidateFs::Ntfs), plan), OK);
    }
    WriteMem dev(mbr_disk());
    dev.lock_rc_ = ERR_VOLUME_LOCK_FAILED; /* a busy volume */
    repair::UndoLog undo;
    EXPECT_EQ(execute_apply(&dev, plan, allow, nullptr, undo), ERR_VOLUME_LOCK_FAILED);
    EXPECT_EQ(dev.committed_, 0u) << "S-010: a busy volume refuses the apply, never races";
    EXPECT_TRUE(undo.entries.empty());
}

TEST(Apply, WriteFailureRollsBackEverything) {
    ApplyPlan plan;
    MemDisk disk;
    {
        /* no signature: the plan has TWO actions (restore 0x55AA + the
         * entry) so a mid-transaction failure has something to roll back */
        std::vector<u8> base = mbr_disk();
        base[510] = 0;
        base[511] = 0; /* signature destroyed */
        MemDisk d;
        d.raw() = base;
        ASSERT_EQ(plan_apply(&d, cand(4096, 512, CandidateFs::Ntfs), plan), OK);
        ASSERT_FALSE(plan.refused);
        ASSERT_EQ(plan.actions.size(), 2u) << "signature restore + entry";
        disk.raw() = base;
    }
    WriteMem dev(disk.raw());
    dev.fail_nth_write_ = 2; /* the entry write fails after the sig restore */
    repair::UndoLog undo;
    EXPECT_EQ(execute_apply(&dev, plan, allow, nullptr, undo), ERR_WRITE_FAILED);
    EXPECT_EQ(dev.raw(), dev.original())
        << "the failed transaction rolled back the signature restore too";
    EXPECT_TRUE(undo.entries.empty()) << "the rolled-back run leaves no undo residue";
}

TEST(Apply, ContainmentRefusals) {
    MemDisk disk;
    disk.raw() = mbr_disk();

    struct Case {
        const char* name;
        lost::Candidate c;
        const char* needle;
    };
    const Case cases[] = {
        {"overlaps the live entry", cand(2048, 10, CandidateFs::Fat), "overlaps"},
        {"starts at LBA 0", cand(0, 100, CandidateFs::Fat), "LBA 0"},
        {"extends past the disk end", cand(15000, 4096, CandidateFs::Fat), "past the end"},
    };
    for (const auto& tc : cases) {
        ApplyPlan plan;
        ASSERT_EQ(plan_apply(&disk, tc.c, plan), OK) << tc.name;
        EXPECT_TRUE(plan.refused) << tc.name;
        EXPECT_NE(plan.refusal_reason.find(tc.needle), std::string::npos)
            << tc.name << ": " << plan.refusal_reason;
        EXPECT_TRUE(plan.actions.empty()) << tc.name << " refuses with no actions";
    }

    /* a full table refuses */
    {
        MemDisk full;
        full.raw()[510] = 0x55;
        full.raw()[511] = 0xAA;
        for (u32 i = 0; i < 4; ++i) {
            u8* e = full.raw().data() + 446 + i * 16;
            e[4] = 0x07;
            u32 lba = 2048u * (i + 1), cnt = 100;
            std::memcpy(e + 8, &lba, 4);
            std::memcpy(e + 12, &cnt, 4);
        }
        ApplyPlan plan;
        ASSERT_EQ(plan_apply(&full, cand(15000, 500, CandidateFs::Fat), plan), OK);
        EXPECT_TRUE(plan.refused);
        EXPECT_NE(plan.refusal_reason.find("full"), std::string::npos);
    }

    /* a GPT (protective MBR) disk refuses cleanly */
    {
        MemDisk gpt;
        gpt.raw()[510] = 0x55;
        gpt.raw()[511] = 0xAA;
        gpt.raw()[446 + 4] = 0xEE;
        ApplyPlan plan;
        ASSERT_EQ(plan_apply(&gpt, cand(4096, 512, CandidateFs::Ntfs), plan), OK);
        EXPECT_TRUE(plan.refused);
        EXPECT_NE(plan.refusal_reason.find("GPT"), std::string::npos);
    }

    /* a zero-size candidate refuses */
    {
        ApplyPlan plan;
        ASSERT_EQ(plan_apply(&disk, cand(4096, 0, CandidateFs::Ntfs), plan), OK);
        EXPECT_TRUE(plan.refused);
    }
}

TEST(Apply, SignatureRestorePlannedWhenAbsent) {
    MemDisk disk;
    disk.raw() = mbr_disk();
    disk.raw()[510] = 0;
    disk.raw()[511] = 0; /* the signature is gone (the og rewrites it) */
    ApplyPlan plan;
    ASSERT_EQ(plan_apply(&disk, cand(4096, 512, CandidateFs::Fat), plan), OK);
    ASSERT_FALSE(plan.refused);
    EXPECT_TRUE(plan.restores_signature);
    ASSERT_EQ(plan.actions.size(), 2u);
    EXPECT_EQ(plan.actions[0].offset, 510u); /* the signature first */
    ASSERT_EQ(plan.actions[0].after.size(), 2u);
    EXPECT_EQ(plan.actions[0].after[0], 0x55);
    EXPECT_EQ(plan.actions[0].after[1], 0xAA);

    WriteMem dev(disk.raw());
    repair::UndoLog undo;
    ASSERT_EQ(execute_apply(&dev, plan, allow, nullptr, undo), OK);
    EXPECT_EQ(dev.raw()[510], 0x55);
    EXPECT_EQ(dev.raw()[511], 0xAA);
    EXPECT_EQ(rd32(dev.raw().data() + 446 + 16 + 8), 4096u) << "the entry also landed";
    ASSERT_EQ(repair::undo(dev, undo), OK);
    EXPECT_EQ(dev.raw(), dev.original());
}

TEST(Apply, ChsClampsBeyondCylinder1023) {
    HugeDisk disk(1024ull * 1024 * 1024); /* 512 GiB geometry, one real sector */
    disk.sector0()[510] = 0x55;
    disk.sector0()[511] = 0xAA;
    /* LBA 16,514,064 = cylinder 1024 exactly -> max CHS on the 255/63
     * geometry: the clamp, not the wraparound */
    ApplyPlan plan;
    ASSERT_EQ(plan_apply(&disk, cand(16514064, 1024, CandidateFs::Ntfs), plan), OK);
    ASSERT_FALSE(plan.refused) << plan.refusal_reason;
    const u8* e = plan.entry_bytes.data();
    EXPECT_EQ(e[1], 0xFE) << "start CHS = max";
    EXPECT_EQ(e[2], 0xFF);
    EXPECT_EQ(e[3], 0xFF);
    EXPECT_EQ(rd32(e + 8), 16514064u) << "the LBA field carries the truth";

    /* inside cylinder 1023: real CHS math (LBA 160650 = cyl 10, head 0,
     * sector 1 on 255/63) */
    ApplyPlan small;
    ASSERT_EQ(plan_apply(&disk, cand(160650, 63, CandidateFs::Ntfs), small), OK);
    ASSERT_FALSE(small.refused);
    const u8* s = small.entry_bytes.data();
    EXPECT_EQ(s[1], 0x00); /* head 0 */
    EXPECT_EQ(s[2], 0x01) << "sector 1 | (cyl 10 >> 8 = 0) -> 0x01";
    EXPECT_EQ(s[3], 0x0A) << "cylinder 10 low byte";
}

TEST(Apply, FamilyTypeMap) {
    EXPECT_EQ(mbr_type_for_family(CandidateFs::Ntfs), 0x07);
    EXPECT_EQ(mbr_type_for_family(CandidateFs::Exfat), 0x07);
    EXPECT_EQ(mbr_type_for_family(CandidateFs::Refs), 0x07);
    EXPECT_EQ(mbr_type_for_family(CandidateFs::Fat), 0x06);
    EXPECT_EQ(mbr_type_for_family(CandidateFs::Ext2), 0x83);
    EXPECT_EQ(mbr_type_for_family(CandidateFs::Btrfs), 0x83);
    EXPECT_EQ(mbr_type_for_family(CandidateFs::HfsPlus), 0xAF);
    EXPECT_EQ(mbr_type_for_family(CandidateFs::Zfs), 0xBF);
    EXPECT_EQ(mbr_type_for_family(CandidateFs::Unknown), 0x07);
}
