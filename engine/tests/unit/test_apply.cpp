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
#include "cr/cr_error.h"
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
class WriteMem final : public apply::IApplyDevice {
public:
    explicit WriteMem(std::vector<u8> backing) : data_(std::move(backing)), orig_(data_) {}
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

bool allow(const ApplyPlan&, void*) {
    return true;
}
bool deny(const ApplyPlan&, void*) {
    return false;
}

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
    ASSERT_EQ(plan.actions.size(), 1u);            /* signature intact: no restore */
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

/* ------------------------------------------------------------------ */
/* The DFX16 convergent-leg proofs (union): the corpus dual-path device
 * sequence, the GPT rebuild variant, the refresh-tail honesty, the race
 * guard, and the plan-is-read-only pin. Adapted to the union API
 * (ApplyConsentFn bool, ERR_APPLY_STALE, entry_index, OK+refused). */
/* ------------------------------------------------------------------ */

namespace {

/* Recording sequence tags - the corpus device-control order. */
enum Op : u8 {
    OP_LOCK,
    OP_UNLOCK,
    OP_DELETE_LAYOUT,
    OP_WRITE,
    OP_UPDATE_PROPS,
    OP_SET_LAYOUT,
    OP_SET_PART_INFO,
    OP_SETTLE,
    OP_GENERATE_SIG,
};

class RecordingApplyDevice : public apply::IApplyDevice, public IStorageDevice {
public:
    std::vector<u8> d_;
    std::vector<u8> ops_; /* Op tags in call order */
    u32 update_rc = OK;
    u32 update_calls = 0;
    u32 settle_calls = 0;
    bool fail_write = false;
    bool corrupt_next_write = false; /* the NEXT write lands mutated (one-shot) */
    u32 lock_rc = OK;
    bool write_before_lock = false;
    u32 generated_signature = 0xC0FFEE01u;
    bool legacy = false;

    explicit RecordingApplyDevice(std::vector<u8> d) : d_(std::move(d)) {
        g_.logical_sector = 512;
        g_.physical_sector = 512;
        g_.total_bytes = d_.size();
        g_.sector_count = d_.size() / 512;
    }

    /* IStorageDevice (planning view) + IWriteDevice (execution view) */
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > d_.size() || len > d_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        memcpy(buf, d_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return g_; }
    u32 write_at(u64 off, const void* buf, u32 len) override {
        if (!locked_)
            write_before_lock = true; /* contract violation if observed */
        if (off > d_.size() || len > d_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (fail_write)
            return ERR_WRITE_FAILED;
        memcpy(d_.data() + off, buf, len);
        if (corrupt_next_write) { /* ONE-SHOT: rollback writes land clean */
            d_[size_t(off)] ^= 0xFF;
            corrupt_next_write = false;
        }
        ops_.push_back(OP_WRITE);
        return OK;
    }
    u64 size() const override { return d_.size(); }
    u32 lock_volume() override {
        ops_.push_back(OP_LOCK);
        locked_ = (lock_rc == OK);
        return lock_rc;
    }
    void unlock_volume() override {
        locked_ = false;
        ops_.push_back(OP_UNLOCK);
    }
    u32 delete_drive_layout() override {
        ops_.push_back(OP_DELETE_LAYOUT);
        return OK;
    }
    u32 set_drive_layout_mbr(u32, u32) override {
        ops_.push_back(OP_SET_LAYOUT);
        return OK;
    }
    u32 set_partition_info_ex(const apply::IApplyDevice::PartitionInfoExRecord& r) override {
        last_record = r;
        ops_.push_back(OP_SET_PART_INFO);
        return OK;
    }
    u32 update_properties() override {
        ops_.push_back(OP_UPDATE_PROPS);
        ++update_calls;
        return update_rc;
    }
    void settle_refresh() override {
        ++settle_calls;
        ops_.push_back(OP_SETTLE);
    }
    bool legacy_os_path() const override { return false; }
    u32 generate_disk_signature() override {
        ops_.push_back(OP_GENERATE_SIG);
        return generated_signature;
    }

    apply::IApplyDevice::PartitionInfoExRecord last_record;

private:
    bool locked_ = false;
    Geometry g_{};
};

/* An MBR disk: one live NTFS entry (slot 0, LBA 2048/1024), signature
 * 0xAABBCCDD, everything else zero. */
std::vector<u8> union_mbr_disk() {
    std::vector<u8> d(size_t(4096) * 512, 0);
    u8* s0 = d.data();
    const auto put32l = [](u8* p, unsigned long v) {
        p[0] = static_cast<u8>(v & 0xFF);
        p[1] = static_cast<u8>((v >> 8) & 0xFF);
        p[2] = static_cast<u8>((v >> 16) & 0xFF);
        p[3] = static_cast<u8>((v >> 24) & 0xFF);
    };
    put32l(s0 + 440, 0xAABBCCDDul);
    s0[446 + 0] = 0x00;
    s0[446 + 4] = 0x07;
    put32l(s0 + 446 + 8, 2048);
    put32l(s0 + 446 + 12, 1024);
    s0[510] = 0x55;
    s0[511] = 0xAA;
    return d;
}

/* A GPT-style sector 0: protective entry (0xEE) filling the disk. */
std::vector<u8> union_gpt_disk() {
    std::vector<u8> d = union_mbr_disk();
    memset(d.data() + 446, 0, 64);
    d[446 + 4] = 0xEE;
    const auto put32l = [](u8* p, unsigned long v) {
        p[0] = static_cast<u8>(v & 0xFF);
        p[1] = static_cast<u8>((v >> 8) & 0xFF);
        p[2] = static_cast<u8>((v >> 16) & 0xFF);
        p[3] = static_cast<u8>((v >> 24) & 0xFF);
    };
    put32l(d.data() + 446 + 8, 1);
    put32l(d.data() + 446 + 12, 4095);
    return d;
}

bool union_approve(const apply::ApplyPlan&, void*) {
    return true;
}

} // namespace

TEST(LostApply, WritesEntryAndRefreshesModernPath) {
    /* the modern path end-to-end: LOCK -> WRITE (sector-granular span) ->
     * SET_PART_INFO (0x7C058 record, slot+1 identity) -> settle+refresh
     * (first succeeds) -> UNLOCK; the live entry preserved; the undo log
     * holds the original sector 0. */
    RecordingApplyDevice dev(union_mbr_disk());
    const std::vector<u8> pristine = dev.d_;
    apply::ApplyPlan plan;
    lost::Candidate c;
    c.start_lba = 3200;
    c.sector_count = 512;
    c.fs = CandidateFs::Ntfs;
    ASSERT_EQ(plan_apply(&dev, c, plan), OK);
    ASSERT_FALSE(plan.refused) << plan.refusal_reason;

    repair::UndoLog undo;
    ASSERT_EQ(execute_apply(&dev, plan, union_approve, nullptr, undo), OK);

    const u8* s0 = dev.d_.data();
    EXPECT_EQ(s0[446 + 16 + 4], 0x07);
    EXPECT_EQ(rd32(s0 + 446 + 16 + 8), 3200u);
    EXPECT_EQ(rd32(s0 + 446 + 16 + 12), 512u);
    EXPECT_EQ(s0[446 + 16 + 0], 0x00); /* boot flag off */
    EXPECT_EQ(s0[446 + 4], 0x07);      /* live entry preserved */
    EXPECT_EQ(rd32(s0 + 440), 0xAABBCCDDu);
    EXPECT_EQ(s0[510], 0x55);
    EXPECT_EQ(s0[511], 0xAA);

    EXPECT_EQ(dev.last_record.partition_number, 2u);
    u64 id_lba = 0, id_cnt = 0;
    memcpy(&id_lba, dev.last_record.identity, 8);
    memcpy(&id_cnt, dev.last_record.identity + 8, 8);
    EXPECT_EQ(id_lba, 3200u);
    EXPECT_EQ(id_cnt, 512u);
    EXPECT_EQ(dev.update_calls, 1u);

    ASSERT_EQ(undo.entries.size(), 1u);
    EXPECT_EQ(undo.entries[0].offset, 0u);
    EXPECT_EQ(undo.entries[0].length, 512u);
    EXPECT_EQ(memcmp(undo.entries[0].original.data(), pristine.data(), 512), 0);
}

TEST(LostApply, LegacyPathMatchesCorpusSequence) {
    /* corpus FUN_1800566d0 legacy order: LOCK -> DELETE_LAYOUT -> WRITE
     * sector 0 -> UPDATE_PROPERTIES -> SET_DRIVE_LAYOUT -> tail
     * (settle+update, break on success) -> UNLOCK at scope exit. */
    struct LegacyDev : RecordingApplyDevice {
        explicit LegacyDev(std::vector<u8> d) : RecordingApplyDevice(std::move(d)) {}
        bool legacy_os_path() const override { return true; }
    };
    LegacyDev dev(union_mbr_disk());
    apply::ApplyPlan plan;
    lost::Candidate c;
    c.start_lba = 3200;
    c.sector_count = 512;
    c.fs = CandidateFs::Ntfs;
    ASSERT_EQ(plan_apply(&dev, c, plan), OK);

    repair::UndoLog undo;
    ASSERT_EQ(execute_apply(&dev, plan, union_approve, nullptr, undo), OK);

    const std::vector<u8> expect = {
        OP_LOCK,                           /* A5 quiesce */
        OP_DELETE_LAYOUT,                  /* 0x7C100 */
        OP_WRITE,                          /* the sector-0 span */
        OP_UPDATE_PROPS,                   /* single refresh (0x70140) */
        OP_SET_LAYOUT,                     /* 0x7C010 */
        OP_SETTLE,        OP_UPDATE_PROPS, /* 3x tail, first succeeds */
        OP_UNLOCK,
    };
    ASSERT_EQ(dev.ops_.size(), expect.size());
    for (size_t i = 0; i < expect.size(); ++i)
        EXPECT_EQ(dev.ops_[i], expect[i]) << "op " << i;
    EXPECT_EQ(dev.d_[446 + 16 + 4], 0x07);
}

TEST(LostApply, GptRebuildRegeneratesSignature) {
    /* corpus FUN_1800392f0: GPT_GenerateSignature -> protective-MBR
     * rewrite (zero entries + 55 AA + NEW 32-bit signature at +440); GPT
     * headers untouched. The finding's fix plan called this "new disk
     * GUID" - corrected against the corpus (see the finding Resolution):
     * every call site passes a 4-byte slot (PR:32805 local_24 ulong). */
    RecordingApplyDevice dev(union_gpt_disk());
    const std::vector<u8> before = dev.d_;
    apply::ApplyPlan plan;
    lost::Candidate c;
    c.start_lba = 3200;
    c.sector_count = 512;
    c.fs = CandidateFs::Ntfs;
    ASSERT_EQ(plan_apply(&dev, c, plan, {}, apply::ApplyScope::RebuildTable), OK);
    ASSERT_FALSE(plan.refused) << plan.refusal_reason;
    EXPECT_TRUE(plan.regenerate_signature); /* forced on GPT (corpus parity) */
    ASSERT_EQ(plan.actions.size(), 1u);     /* the one full-sector rebuild */
    EXPECT_EQ(plan.actions[0].offset, 0u);
    EXPECT_EQ(plan.actions[0].after.size(), 512u);

    repair::UndoLog undo;
    ASSERT_EQ(execute_apply(&dev, plan, union_approve, nullptr, undo), OK);

    const u8* s0 = dev.d_.data();
    EXPECT_EQ(rd32(s0 + 440), 0xC0FFEE01u); /* the generated signature */
    EXPECT_NE(memcmp(s0 + 440, before.data() + 440, 4), 0);
    EXPECT_EQ(s0[446 + 4], 0x07); /* candidate at slot 0 */
    EXPECT_EQ(rd32(s0 + 446 + 8), 3200u);
    EXPECT_EQ(rd32(s0 + 446 + 12), 512u);
    EXPECT_EQ(s0[446 + 16 + 4], 0); /* slots 1..3 zeroed */
    EXPECT_EQ(s0[446 + 32 + 4], 0);
    EXPECT_EQ(s0[446 + 48 + 4], 0);
    EXPECT_EQ(s0[510], 0x55);
    EXPECT_EQ(s0[511], 0xAA);
    /* the boot code area and everything past sector 0 untouched */
    EXPECT_EQ(memcmp(s0, before.data(), 440), 0);
    EXPECT_EQ(memcmp(dev.d_.data() + 512, before.data() + 512, dev.d_.size() - 512), 0);
    EXPECT_NE(std::find(dev.ops_.begin(), dev.ops_.end(), OP_GENERATE_SIG), dev.ops_.end());
}

TEST(LostApply, ConsentBeforeLockDenialHoldsNothing) {
    /* A1 (the union order): a denial takes NO lock - the caller
     * deliberates without the volume being held. */
    RecordingApplyDevice dev(union_mbr_disk());
    apply::ApplyPlan plan;
    lost::Candidate c;
    c.start_lba = 3200;
    c.sector_count = 512;
    c.fs = CandidateFs::Ntfs;
    ASSERT_EQ(plan_apply(&dev, c, plan), OK);

    repair::UndoLog undo;
    EXPECT_EQ(execute_apply(&dev, plan, nullptr, nullptr, undo), ERR_APPLY_CONSENT_DENIED);
    EXPECT_EQ(execute_apply(
                  &dev, plan, [](const apply::ApplyPlan&, void*) { return false; }, nullptr, undo),
              ERR_APPLY_CONSENT_DENIED);
    EXPECT_TRUE(std::find(dev.ops_.begin(), dev.ops_.end(), OP_LOCK) == dev.ops_.end());
    EXPECT_TRUE(std::find(dev.ops_.begin(), dev.ops_.end(), OP_WRITE) == dev.ops_.end());
    EXPECT_EQ(dev.update_calls, 0u);
    EXPECT_EQ(memcmp(dev.d_.data(), union_mbr_disk().data(), 512), 0);
    EXPECT_TRUE(undo.entries.empty());
}

TEST(LostApply, UndoApplyRestoresAndRefreshes) {
    RecordingApplyDevice dev(union_mbr_disk());
    const std::vector<u8> pristine = dev.d_;
    apply::ApplyPlan plan;
    lost::Candidate c;
    c.start_lba = 3200;
    c.sector_count = 512;
    c.fs = CandidateFs::Ntfs;
    ASSERT_EQ(plan_apply(&dev, c, plan), OK);

    repair::UndoLog undo;
    ASSERT_EQ(execute_apply(&dev, plan, union_approve, nullptr, undo), OK);
    EXPECT_NE(memcmp(dev.d_.data(), pristine.data(), 512), 0);

    const u32 updates_before = dev.update_calls;
    ASSERT_EQ(undo_apply(dev, undo), OK);
    EXPECT_EQ(memcmp(dev.d_.data(), pristine.data(), 512), 0);
    EXPECT_EQ(dev.update_calls, updates_before + 1u);
    /* idempotent second undo */
    EXPECT_EQ(undo_apply(dev, undo), OK);
    EXPECT_EQ(memcmp(dev.d_.data(), pristine.data(), 512), 0);
}

TEST(LostApply, RefreshFailureIsReportedNotSwallowed) {
    /* corpus PR:47015-47025: the og IGNORES a failing refresh tail. The
     * core reports ERR_APPLY_REFRESH_FAILED with the table written and
     * verified, the undo log still valid (D-72c). */
    RecordingApplyDevice dev(union_mbr_disk());
    dev.update_rc = ERR_INTERNAL;
    apply::ApplyPlan plan;
    lost::Candidate c;
    c.start_lba = 3200;
    c.sector_count = 512;
    c.fs = CandidateFs::Ntfs;
    ASSERT_EQ(plan_apply(&dev, c, plan), OK);

    repair::UndoLog undo;
    EXPECT_EQ(execute_apply(&dev, plan, union_approve, nullptr, undo), ERR_APPLY_REFRESH_FAILED);
    EXPECT_EQ(dev.d_[446 + 16 + 4], 0x07); /* the table IS applied */
    EXPECT_EQ(dev.settle_calls, 3u);       /* exactly 3 tail attempts */
    EXPECT_EQ(dev.update_calls, 3u);
    ASSERT_EQ(undo.entries.size(), 1u);
    EXPECT_EQ(undo_apply(dev, undo), ERR_APPLY_REFRESH_FAILED); /* restore ok, refresh stale */
    EXPECT_EQ(memcmp(dev.d_.data(), union_mbr_disk().data(), 512), 0);
}

TEST(LostApply, RollsBackWhenVerifyFails) {
    /* A4: a write that lands wrong triggers rollback - sector 0 restored
     * byte-exact, the log left empty. */
    RecordingApplyDevice dev(union_mbr_disk());
    dev.corrupt_next_write = true;
    apply::ApplyPlan plan;
    lost::Candidate c;
    c.start_lba = 3200;
    c.sector_count = 512;
    c.fs = CandidateFs::Ntfs;
    ASSERT_EQ(plan_apply(&dev, c, plan), OK);

    repair::UndoLog undo;
    EXPECT_EQ(execute_apply(&dev, plan, union_approve, nullptr, undo), ERR_REPAIR_REFUSED);
    EXPECT_EQ(memcmp(dev.d_.data(), union_mbr_disk().data(), 512), 0);
    EXPECT_TRUE(undo.entries.empty());
}

TEST(LostApply, PlanIsReadOnlyAndRefusalsAreResults) {
    /* the planning phase performs ZERO writes and ZERO device controls;
     * refusals are OK+refused (the union convention), never error codes. */
    RecordingApplyDevice dev(union_mbr_disk());
    apply::ApplyPlan plan;
    lost::Candidate c;
    c.start_lba = 3200;
    c.sector_count = 512;
    c.fs = CandidateFs::Ntfs;
    ASSERT_EQ(plan_apply(&dev, c, plan), OK);
    EXPECT_TRUE(dev.ops_.empty());
    EXPECT_EQ(memcmp(dev.d_.data(), union_mbr_disk().data(), 512), 0);
    EXPECT_EQ(plan.entry_index, 1u);
    EXPECT_EQ(plan.mbr_type, 0x07);
    EXPECT_EQ(plan.mbr_signature, 0xAABBCCDDu);
    EXPECT_EQ(plan.record.partition_number, 2u);
    EXPECT_EQ(plan.sector0_original.size(), 512u);
    EXPECT_EQ(plan.scope, apply::ApplyScope::SingleEntry);

    /* the 32-bit-LBA-range refusal (the union's A3 addition) - on a
     * huge-disk device so the capacity check does not fire first */
    HugeDisk big(64ull * 1024 * 1024 * 1024 * 1024 / 512); /* 64 TiB */
    big.sector0()[510] = 0x55;
    big.sector0()[511] = 0xAA;
    lost::Candidate huge;
    huge.start_lba = 0xFFFFFF00ull;
    huge.sector_count = 0x2000; /* end = 0x1_00001000 > the 32-bit range */
    huge.fs = CandidateFs::Ntfs;
    apply::ApplyPlan hp;
    EXPECT_EQ(plan_apply(&big, huge, hp), OK);
    EXPECT_TRUE(hp.refused);
    EXPECT_NE(hp.refusal_reason.find("32-bit MBR LBA range"), std::string::npos);
}
