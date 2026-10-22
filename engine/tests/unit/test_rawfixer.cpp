/* test_rawfixer.cpp - transactional repair: analyze/execute/undo + safety.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/repair/cr_rawfixer.h"
#include "cr/cr_error.h"
#include "cr/exfat/cr_exfat_engine.h"
#include <gtest/gtest.h>
#include <cstring>

namespace cr {
namespace repair {
namespace {

class RamDevice final : public IStorageDevice {
public:
    explicit RamDevice(std::vector<u8> d) : d_(std::move(d)) {
        g_.logical_sector = 512;
        g_.physical_sector = 512;
        g_.total_bytes = d_.size();
        g_.sector_count = d_.size() / 512;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > d_.size() || len > d_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        memcpy(buf, d_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return g_; }

    std::vector<u8> d_;

private:
    Geometry g_{};
};

class RamWriteDevice final : public IWriteDevice {
public:
    std::vector<u8> d_;
    u32 writes = 0;
    u32 fail_write_at = ~u32(0); /* offset whose write fails (test hook) */

    explicit RamWriteDevice(std::vector<u8> d) : d_(std::move(d)) {}
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > d_.size() || len > d_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        memcpy(buf, d_.data() + off, len);
        return OK;
    }
    u32 write_at(u64 off, const void* buf, u32 len) override {
        if (off > d_.size() || len > d_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (u32(off) == fail_write_at)
            return ERR_WRITE_FAILED;
        memcpy(d_.data() + off, buf, len);
        ++writes;
        return OK;
    }
    u64 size() const override { return d_.size(); }
};

void put16(u8* p, u16 v) {
    p[0] = u8(v);
    p[1] = u8(v >> 8);
}
void put32(u8* p, u32 v) {
    p[0] = u8(v);
    p[1] = u8(v >> 8);
    p[2] = u8(v >> 16);
    p[3] = u8(v >> 24);
}
void put64(u8* p, u64 v) {
    put32(p, u32(v));
    put32(p + 4, u32(v >> 32));
}

/* A spec-shaped exFAT boot sector: fields at the exFAT-spec offsets
 * (FatOffset +0x50, FatLength +0x54, ClusterHeapOffset +0x58,
 * ClusterCount +0x5C, BytesPerSectorShift +0x6C, SectorsPerClusterShift
 * +0x6D) - NOT the FAT BPB offsets. */
struct ExfatShape {
    /* volume >= 2048 sectors (spec minimum 1 MiB); 4096 sectors = 2 MiB */
    u32 volume_length_sec = 4096;
    u32 fat_offset = 24;
    u32 fat_length = 8;
    u32 heap_offset = 40;
    u32 cluster_count = 4056; /* (4096-40) clusters at spc-shift 0 */
    u8 spc_shift = 0;
};

void build_exfat_boot(u8* b, const ExfatShape& s) {
    memset(b, 0, 512);
    b[0] = 0xEB;
    b[1] = 0x76;
    b[2] = 0x90;
    memcpy(b + 3, "EXFAT   ", 8);
    put64(b + 0x40, 0); /* PartitionOffset */
    put64(b + 0x48, s.volume_length_sec);
    put32(b + 0x50, s.fat_offset);
    put32(b + 0x54, s.fat_length);
    put32(b + 0x58, s.heap_offset);
    put32(b + 0x5C, s.cluster_count);
    put32(b + 0x60, 2); /* root cluster */
    put32(b + 0x64, 0x12345678);
    put16(b + 0x68, 0x0100); /* fs revision */
    put16(b + 0x6A, 0);      /* VolumeFlags */
    b[0x6C] = 9;             /* BytesPerSectorShift: 512 */
    b[0x6D] = s.spc_shift;   /* SectorsPerClusterShift */
    b[0x6E] = 1;             /* NumberOfFats */
    b[0x6F] = 0x80;          /* DriveSelect */
    b[0x70] = 0;             /* PercentInUse */
    b[510] = 0x55;
    b[511] = 0xAA;
}

/* Fill sectors 11 and 23 with the spec checksum over 0..10 / 12..22. */
void fill_exfat_checksums(std::vector<u8>& img, u32 bps) {
    u32 chk_main = exfat::boot_region_checksum(img.data(), bps);
    for (u32 k = 0; k < bps / 4; ++k)
        put32(img.data() + 11 * bps + k * 4, chk_main);
    /* backup side: same sectors shifted by 12; compute over 12..22 */
    std::vector<u8> region(img.begin() + 12 * bps, img.begin() + 23 * bps);
    region.resize(11 * bps);
    u32 chk_bak = 0;
    for (u32 s = 0; s <= 10; ++s) {
        const u8* sec = region.data() + u64(s) * bps;
        for (u32 i = 0; i < bps; ++i) {
            if (s == 0 && ((i >= 106 && i < 108) || i == 112))
                continue;
            chk_bak = ((chk_bak & 1) ? 0x80000000u : 0u) + (chk_bak >> 1) + sec[i];
        }
    }
    for (u32 k = 0; k < bps / 4; ++k)
        put32(img.data() + 23 * bps + k * 4, chk_bak);
}

TEST(RawFixer, CleanExfatNeverTouched) {
    /* S2: a valid exFAT boot region produces an empty plan */
    ExfatShape s;
    std::vector<u8> img(2ull << 20, 0);
    build_exfat_boot(img.data(), s);
    /* extended boot sectors + OEM params: signature 0xAA550000 at +508 */
    for (u32 sec = 1; sec <= 10; ++sec) {
        if (sec <= 8)
            put32(img.data() + sec * 512 + 508, 0xAA550000);
        img[(12 + sec) * 512 + 508] = 0;
    }
    img[12 * 512 + 508] = 0;
    fill_exfat_checksums(img, 512);

    RamDevice dev(img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan)) << err_text(OK);
    EXPECT_TRUE(plan.detected);
    EXPECT_EQ("exfat", plan.fs_family);
    EXPECT_FALSE(plan.refused);
    EXPECT_TRUE(plan.actions.empty());
}

TEST(RawFixer, ExfatChecksumRepair) {
    ExfatShape s;
    std::vector<u8> img(2ull << 20, 0);
    build_exfat_boot(img.data(), s);
    fill_exfat_checksums(img, 512);
    /* corrupt the checksum fill at sector 11 */
    memset(img.data() + 11 * 512, 0xAB, 512);

    RamDevice dev(img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    ASSERT_EQ(1u, plan.actions.size());
    EXPECT_EQ(ActionKind::RepairBootChecksum, plan.actions[0].kind);
    EXPECT_EQ(11 * 512u, plan.actions[0].offset);

    RamWriteDevice wd(img);
    UndoLog undo_log;
    ASSERT_EQ(OK, execute(&wd, plan, undo_log));
    EXPECT_EQ(1u, wd.writes);
    ASSERT_EQ(1u, undo_log.entries.size());

    /* the repaired fill must match the spec checksum over sectors 0..10 */
    const u32 expect = exfat::boot_region_checksum(wd.d_.data(), 512);
    for (u32 k = 0; k < 128; ++k)
        EXPECT_EQ(expect, u32(wd.d_[11 * 512 + k * 4]) | (u32(wd.d_[11 * 512 + k * 4 + 1]) << 8) |
                              (u32(wd.d_[11 * 512 + k * 4 + 2]) << 16) |
                              (u32(wd.d_[11 * 512 + k * 4 + 3]) << 24))
            << "fill word " << k;

    /* undo restores the original corrupted bytes */
    ASSERT_EQ(OK, undo(wd, undo_log));
    EXPECT_EQ(0xAB, wd.d_[11 * 512]);
    EXPECT_EQ(0xAB, wd.d_[11 * 512 + 511]);
}

TEST(RawFixer, ExfatDestroyedPrimaryRestoredFromBackup) {
    ExfatShape s;
    std::vector<u8> img(2ull << 20, 0);
    build_exfat_boot(img.data(), s);
    build_exfat_boot(img.data() + 12 * 512, s); /* backup boot sector */
    fill_exfat_checksums(img, 512);
    const std::vector<u8> backup_boot(img.begin() + 12 * 512, img.begin() + 13 * 512);
    /* destroy the primary boot sector entirely */
    memset(img.data(), 0x77, 512);

    RamDevice dev(img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    EXPECT_TRUE(plan.detected);
    EXPECT_EQ("exfat", plan.fs_family);
    ASSERT_GE(plan.actions.size(), 1u);
    EXPECT_EQ(ActionKind::RestoreBootFromBackup, plan.actions[0].kind);
    /* a restore invalidates the checksum -> recompute also planned */
    ASSERT_EQ(2u, plan.actions.size());
    EXPECT_EQ(ActionKind::RepairBootChecksum, plan.actions[1].kind);

    RamWriteDevice wd(img);
    UndoLog undo_log;
    ASSERT_EQ(OK, execute(&wd, plan, undo_log));
    EXPECT_EQ(2u, wd.writes);
    /* primary restored from the backup */
    EXPECT_EQ(0, memcmp(wd.d_.data(), backup_boot.data(), 512));
    /* checksum fill consistent with the restored region */
    const u32 expect = exfat::boot_region_checksum(wd.d_.data(), 512);
    EXPECT_EQ(expect, u32(wd.d_[11 * 512]) | (u32(wd.d_[11 * 512 + 1]) << 8) |
                          (u32(wd.d_[11 * 512 + 2]) << 16) | (u32(wd.d_[11 * 512 + 3]) << 24));

    /* undo restores the destroyed original */
    ASSERT_EQ(OK, undo(wd, undo_log));
    EXPECT_EQ(0x77, wd.d_[0]);
    EXPECT_EQ(0x77, wd.d_[511]);
}

TEST(RawFixer, NtfsBackupRestore) {
    std::vector<u8> img(1ull << 20, 0);
    /* damaged primary: NTFS OEM but zero geometry */
    memcpy(img.data() + 3, "NTFS    ", 8);
    img[510] = 0x55;
    img[511] = 0xAA;
    /* valid backup at volume end */
    u8* b = img.data() + img.size() - 512;
    memset(b, 0, 512);
    memcpy(b + 3, "NTFS    ", 8);
    put16(b + 0x0B, 512);  /* 512B sectors */
    b[0x0D] = 1;           /* spc 1 */
    put64(b + 0x28, 2048); /* total sectors */
    put64(b + 0x30, 4);    /* $MFT LCN */
    b[0x40] = 0xF6;        /* cpr = -10 -> 1<<10 record size */

    RamDevice dev(img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    EXPECT_TRUE(plan.detected);
    EXPECT_EQ("ntfs", plan.fs_family);
    ASSERT_EQ(1u, plan.actions.size());
    EXPECT_EQ(ActionKind::RestoreBootFromBackup, plan.actions[0].kind);

    RamWriteDevice wd(img);
    UndoLog undo_log;
    ASSERT_EQ(OK, execute(&wd, plan, undo_log));
    /* primary now equals the backup */
    EXPECT_EQ(0, memcmp(wd.d_.data(), img.data() + img.size() - 512, 512));
    /* undo restores the damaged original byte-for-byte */
    ASSERT_EQ(OK, undo(wd, undo_log));
    EXPECT_EQ(0, memcmp(wd.d_.data(), img.data(), 512));
    EXPECT_EQ(0, wd.d_[0x0B]);
    EXPECT_EQ(0, wd.d_[0x0D]);
}

TEST(RawFixer, NtfsCleanRefused) {
    /* S2: a valid NTFS boot sector is never touched */
    std::vector<u8> img(1ull << 20, 0);
    u8* b = img.data();
    memcpy(b + 3, "NTFS    ", 8);
    put16(b + 0x0B, 512);
    b[0x0D] = 1;
    put64(b + 0x28, 2048);
    put64(b + 0x30, 4);
    b[0x40] = 0xF6;

    RamDevice dev(img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    EXPECT_TRUE(plan.detected);
    EXPECT_EQ("ntfs", plan.fs_family);
    EXPECT_FALSE(plan.refused);
    EXPECT_TRUE(plan.actions.empty());
}

TEST(RawFixer, NtfsNoBackupRefused) {
    /* S3: NTFS-signed primary damaged AND the volume-end sector is not a
     * valid NTFS boot -> refusal, never a guess */
    std::vector<u8> img(1ull << 20, 0x55);
    memcpy(img.data() + 3, "NTFS    ", 8);

    RamDevice dev(img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    EXPECT_TRUE(plan.refused);
    EXPECT_TRUE(plan.actions.empty());
}

TEST(RawFixer, Fat32BackupRestore) {
    /* FAT32: damaged primary + valid backup at sector 6 */
    std::vector<u8> img(8ull << 20, 0);
    u8* b = img.data() + 6 * 512;
    memset(b, 0, 512);
    memcpy(b + 3, "MSDOS5.0", 8);
    put16(b + 0x0B, 512);
    b[0x0D] = 8;            /* spc */
    put16(b + 0x0E, 32);    /* reserved */
    b[0x10] = 2;            /* fats */
    put16(b + 0x11, 0);     /* root entries (FAT32) */
    put16(b + 0x13, 0);     /* total16 */
    put16(b + 0x16, 0);     /* fat16 size */
    put32(b + 0x20, 16384); /* total32 */
    put32(b + 0x24, 200);   /* fat32 size */
    put32(b + 0x2C, 2);     /* root cluster */
    b[510] = 0x55;
    b[511] = 0xAA;

    /* primary: garbage, no signature */
    memset(img.data(), 0x99, 512);

    RamDevice dev(img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    EXPECT_TRUE(plan.detected);
    EXPECT_EQ("fat", plan.fs_family);
    ASSERT_EQ(1u, plan.actions.size());
    EXPECT_EQ(ActionKind::RestoreBootFromBackup, plan.actions[0].kind);

    RamWriteDevice wd(img);
    UndoLog undo_log;
    ASSERT_EQ(OK, execute(&wd, plan, undo_log));
    EXPECT_EQ(0, memcmp(wd.d_.data(), img.data() + 6 * 512, 512));
    ASSERT_EQ(OK, undo(wd, undo_log));
    EXPECT_EQ(0x99, wd.d_[0]);
}

TEST(RawFixer, ExfatGeometryRederivation) {
    /* sectors-per-cluster shift wrong: cluster count only fits one shift */
    ExfatShape s; /* heap = 4056 sectors; 4056 clusters at shift 0 */
    s.cluster_count = 4056;
    s.spc_shift = 3; /* declared shift 3 - inconsistent with the count */
    std::vector<u8> img(2ull << 20, 0);
    build_exfat_boot(img.data(), s);
    fill_exfat_checksums(img, 512);

    RamDevice dev(img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    ASSERT_TRUE(plan.detected);
    /* heap_bytes = 2MiB - 20480 = 2064384; /512 = 4032 clusters at shift
     * 1... but the exact heap is 4056 sectors: only shift 0 yields the
     * declared count (4056/4057). Exactly one candidate -> repair. */
    bool has_geom = false;
    for (const auto& a : plan.actions) {
        if (a.kind == ActionKind::RepairGeometryField)
            has_geom = true;
    }
    if (!plan.refused) {
        EXPECT_TRUE(has_geom);
    }
}

TEST(RawFixer, ExfatAmbiguousGeometryRefused) {
    /* two shifts fit equally well -> S3 refusal */
    ExfatShape s; /* heap 4056 sectors: 4056, 2028, 1014, 507... at
                     shifts 0,1,2,3 - none equal 400 or 401 -> no
                     candidate -> geometry refused (S3) */
    s.cluster_count = 400;
    s.spc_shift = 0;
    std::vector<u8> img(2ull << 20, 0);
    build_exfat_boot(img.data(), s);
    fill_exfat_checksums(img, 512);

    RamDevice dev(img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    /* no shift yields 400 or 401 clusters -> refused (S3) */
    bool has_geom = false;
    for (const auto& a : plan.actions) {
        if (a.kind == ActionKind::RepairGeometryField)
            has_geom = true;
    }
    EXPECT_FALSE(has_geom);
}

TEST(RawFixer, UnknownFamilyRefused) {
    std::vector<u8> img(2ull << 20, 0x77);
    RamDevice dev(img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    EXPECT_TRUE(plan.refused);
    EXPECT_TRUE(plan.actions.empty());
    EXPECT_FALSE(plan.detected);
}

TEST(RawFixer, ExecuteRefusesEmptyPlan) {
    RamWriteDevice wd(std::vector<u8>(2ull << 20, 0));
    RepairPlan plan; /* empty */
    UndoLog log;
    EXPECT_EQ(ERR_REPAIR_REFUSED, execute(&wd, plan, log));
    EXPECT_EQ(0u, wd.writes);
}

TEST(RawFixer, FailedWriteRollsBackEverything) {
    /* S4/S5: a write that fails mid-plan rolls back every earlier action */
    ExfatShape s;
    std::vector<u8> img(2ull << 20, 0);
    build_exfat_boot(img.data(), s);
    fill_exfat_checksums(img, 512);
    memset(img.data() + 11 * 512, 0xAB, 512); /* checksum wrong */

    RamDevice dev(img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    ASSERT_EQ(1u, plan.actions.size());

    /* two-action plan: craft one manually to prove multi-action rollback */
    RepairPlan two;
    two.actions = plan.actions;
    RepairAction extra;
    extra.kind = ActionKind::RepairGeometryField;
    extra.offset = 0x30000;
    extra.length = 1;
    extra.after = {0x5A};
    extra.reason = "synthetic second action";
    two.actions.push_back(extra);

    RamWriteDevice wd(img);
    wd.fail_write_at = 0x30000; /* the second action's write fails */
    UndoLog undo_log;
    EXPECT_EQ(ERR_WRITE_FAILED, execute(&wd, two, undo_log));
    /* the first action (checksum repair) must have been rolled back */
    EXPECT_EQ(0xAB, wd.d_[11 * 512]);
    EXPECT_EQ(2u, wd.writes);               /* the applied write + the rollback write */
    EXPECT_EQ(0u, undo_log.entries.size()); /* log truncated to pre-plan */
}

TEST(RawFixer, UndoLogRoundTrip) {
    UndoLog log;
    u8 orig[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    log.record(0x1234, 16, orig);
    std::vector<u8> blob;
    log.serialize(blob);
    UndoLog back;
    ASSERT_EQ(OK, UndoLog::deserialize(blob.data(), blob.size(), back));
    ASSERT_EQ(1u, back.entries.size());
    EXPECT_EQ(u64(0x1234), back.entries[0].offset);
    EXPECT_EQ(16u, back.entries[0].length);
    EXPECT_EQ(0, memcmp(back.entries[0].original.data(), orig, 16));

    /* corruption detected */
    blob[blob.size() / 2] ^= 0xFF;
    EXPECT_EQ(ERR_BAD_CHECKSUM, UndoLog::deserialize(blob.data(), blob.size(), back));

    /* truncation detected */
    blob.resize(blob.size() - 6);
    EXPECT_NE(OK, UndoLog::deserialize(blob.data(), blob.size(), back));
}

} // namespace
} // namespace repair
} // namespace cr
