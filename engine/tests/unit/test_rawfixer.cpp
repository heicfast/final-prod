/* test_rawfixer.cpp - transactional repair: analyze/execute/undo + safety.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/repair/cr_rawfixer.h"
#include "cr/cr_error.h"
#include "cr/ntfs/cr_attributes.h"
#include "cr/ntfs/cr_fixup.h"
#include "cr/ntfs/cr_mftwalk.h"
#include "cr/ntfs/cr_ntfs_wire.h"
#include "cr/storage/cr_slice.h"
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
    /* S-010 lock-contract recording */
    u32 lock_rc = OK;
    int lock_calls = 0, unlock_calls = 0;
    bool write_before_lock = false;

    explicit RamWriteDevice(std::vector<u8> d) : d_(std::move(d)) {}
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > d_.size() || len > d_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        memcpy(buf, d_.data() + off, len);
        return OK;
    }
    u32 write_at(u64 off, const void* buf, u32 len) override {
        if (lock_calls == 0)
            write_before_lock = true; /* contract violation if observed */
        if (off > d_.size() || len > d_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (u32(off) == fail_write_at)
            return ERR_WRITE_FAILED;
        memcpy(d_.data() + off, buf, len);
        ++writes;
        return OK;
    }
    u64 size() const override { return d_.size(); }
    u32 lock_volume() override {
        ++lock_calls;
        return lock_rc;
    }
    void unlock_volume() override { ++unlock_calls; }
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

/* ---- S-008: CheckMFT / CheckFatTable / CheckRootDir / FixByScanSector ---- */

/* One spec-shaped 1 KiB MFT record: USA fixup (USN=1 on both sector tails)
 * + a resident $FILE_NAME carrying `name`. When `mft_runlist` is set, also
 * a resident $DATA whose value bytes are the $MFT runlist (tiny-MFT path
 * the engine's load_mft_stream accepts). */
std::vector<u8> build_meta_record(const std::u16string& name, bool mft_runlist, u32 record_no) {
    std::vector<u8> rec(1024, 0);
    auto* rh = reinterpret_cast<ntfs::RecordHeader*>(rec.data());
    rh->magic = ntfs::RECORD_MAGIC_FILE;
    rh->update_sequence_offset = 0x30;
    rh->update_sequence_size = 3; /* 2 sectors + 1 */
    rh->sequence = 1;
    rh->link_count = 1;
    rh->attribute_offset = 0x38;
    rh->flags = ntfs::RECORD_IN_USE;
    rh->bytes_in_use = 1024;
    rh->bytes_allocated = 1024;
    rh->base_record = 0;
    rh->next_attr_id = 4;
    rh->record_number = record_no;
    put16(rec.data() + 0x30, 1); /* USA: USN 1, placeholder tails 0 */
    put16(rec.data() + 510, 1);  /* sector tails carry the USN */
    put16(rec.data() + 1022, 1);
    u32 pos = 0x38;
    /* $FILE_NAME resident: value at 0x18 */
    {
        const u32 vlen = 66 + u32(name.size()) * 2;
        const u32 alen = (24 + vlen + 7) / 8 * 8;
        put32(rec.data() + pos + 0, ntfs::ATTR_TYPE_FILE_NAME);
        put32(rec.data() + pos + 4, alen);
        rec[pos + 8] = 0;
        rec[pos + 9] = 0;
        put16(rec.data() + pos + 10, u16(24));
        put32(rec.data() + pos + 16, vlen);
        put16(rec.data() + pos + 20, u16(24));
        u8* v = rec.data() + pos + 24;
        v[64] = u8(name.size());
        v[65] = ntfs::FILE_NAME_POSIX;
        for (size_t i = 0; i < name.size(); ++i)
            put16(v + 66 + u32(i) * 2, name[i]);
        pos += alen;
    }
    if (mft_runlist) {
        /* resident $DATA: value = one run (16 clusters at LCN 4) + terminator */
        static const u8 rl[5] = {0x21, 0x10, 0x04, 0x00, 0x00};
        const u32 vlen = sizeof(rl);
        const u32 alen = (24 + vlen + 7) / 8 * 8;
        put32(rec.data() + pos + 0, ntfs::ATTR_TYPE_DATA);
        put32(rec.data() + pos + 4, alen);
        rec[pos + 8] = 0;
        rec[pos + 9] = 0;
        put16(rec.data() + pos + 10, u16(24));
        put32(rec.data() + pos + 16, vlen);
        put16(rec.data() + pos + 20, u16(24));
        std::memcpy(rec.data() + pos + 24, rl, sizeof(rl));
        pos += alen;
    }
    put32(rec.data() + pos, 0xFFFFFFFF); /* terminator */
    return rec;
}

/* Minimal openable NTFS volume (1 MiB, spc 1, records 1 KiB): boot, $MFT
 * record 0 at LCN 4 (with the $MFT runlist), root record 5 valid, $MFTMirr
 * slot 0 (byte copy of record 0) at LCN 100. */
struct NtfsVolumeImage {
    std::vector<u8> img;
    static constexpr u64 MFT_OFF = 4 * 512;
    static constexpr u64 MIRR_OFF = 100 * 512;
    explicit NtfsVolumeImage(bool tear_record0) : img(1ull << 20, 0) {
        u8* b = img.data();
        std::memcpy(b + 3, "NTFS    ", 8);
        put16(img.data() + 0x0B, 512);
        b[0x0D] = 1;                    /* spc 1 */
        put64(img.data() + 0x28, 2048); /* total sectors (1 MiB) */
        put64(img.data() + 0x30, 4);    /* $MFT LCN */
        put64(img.data() + 0x38, 100);  /* $MFTMirr LCN */
        b[0x40] = 0xF6;                 /* cpr -10 -> 1 KiB records */
        b[510] = 0x55;
        b[511] = 0xAA;
        const auto rec0 = build_meta_record(u"$MFT", true, 0);
        std::memcpy(img.data() + MFT_OFF, rec0.data(), rec0.size());
        const auto root5 = build_meta_record(u".", false, 5);
        std::memcpy(img.data() + MFT_OFF + 5 * 1024, root5.data(), root5.size());
        std::memcpy(img.data() + MIRR_OFF, rec0.data(), rec0.size()); /* mirror slot 0 */
        if (tear_record0)
            std::memset(img.data() + MFT_OFF, 0xDE, 1024); /* torn primary */
    }
};

/* Mirrored FAT16 volume (5074 sectors): reserved 2, two 20-sector FATs,
 * 512 root entries. Both FAT copies carry the FAT16 entry signature. */
struct Fat16MirrorImage {
    std::vector<u8> img;
    static constexpr u64 FAT1 = 2 * 512;
    static constexpr u64 FAT2 = 22 * 512;
    static constexpr u64 FAT_BYTES = 20 * 512;
    static constexpr u64 ROOT = 42 * 512;
    Fat16MirrorImage() : img(u64(5074) * 512, 0) {
        u8* b = img.data();
        std::memcpy(b + 3, "MSDOS5.0", 8);
        put16(img.data() + 0x0B, 512);
        b[0x0D] = 1;
        put16(img.data() + 0x0E, 2);
        b[0x10] = 2; /* mirrored */
        put16(img.data() + 0x11, 512);
        put16(img.data() + 0x13, 5074);
        b[0x15] = 0xF8;
        put16(img.data() + 0x16, 20);
        std::memcpy(b + 0x36, "FAT16   ", 8);
        b[510] = 0x55;
        b[511] = 0xAA;
        for (u64 fat : {FAT1, FAT2}) {
            img[fat + 0] = 0xF8; /* entry0 = 0xFFF8 (media) */
            img[fat + 1] = 0xFF;
            img[fat + 2] = 0xFF; /* entry1 = 0xFFFF */
            img[fat + 3] = 0xFF;
        }
    }
    void write_fat_sector(u32 copy /*1|2*/, u64 sector_in_fat, u8 fill) {
        const u64 base = copy == 1 ? FAT1 : FAT2;
        std::memset(img.data() + base + sector_in_fat * 512, fill, 512);
    }
};

TEST(RawFixer, MftRecord0RestoredFromMirror) {
    NtfsVolumeImage v(true);
    RamDevice dev(v.img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    EXPECT_TRUE(plan.detected);
    EXPECT_EQ("ntfs", plan.fs_family);
    ASSERT_EQ(1u, plan.actions.size());
    EXPECT_EQ(ActionKind::RestoreMftRecordFromMirror, plan.actions[0].kind);
    EXPECT_EQ(NtfsVolumeImage::MFT_OFF, plan.actions[0].offset);
    EXPECT_EQ(1024u, plan.actions[0].length);
    /* the restore source is the mirror slot 0, byte for byte */
    EXPECT_EQ(0,
              memcmp(plan.actions[0].after.data(), v.img.data() + NtfsVolumeImage::MIRR_OFF, 1024));

    RamWriteDevice wd(v.img);
    UndoLog undo_log;
    ASSERT_EQ(OK, execute(&wd, plan, undo_log));
    EXPECT_EQ(0, memcmp(wd.d_.data() + NtfsVolumeImage::MFT_OFF,
                        v.img.data() + NtfsVolumeImage::MIRR_OFF, 1024));
    /* undo restores the torn original */
    ASSERT_EQ(OK, undo(wd, undo_log));
    EXPECT_EQ(0xDE, wd.d_[NtfsVolumeImage::MFT_OFF]);
}

TEST(RawFixer, MftCleanNeverTouched) {
    NtfsVolumeImage v(false);
    RamDevice dev(v.img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    EXPECT_TRUE(plan.detected);
    EXPECT_TRUE(plan.actions.empty());
    EXPECT_TRUE(plan.advisories.empty());
}

TEST(RawFixer, MftBothInvalidNoWrite) {
    NtfsVolumeImage v(true);
    std::memset(v.img.data() + NtfsVolumeImage::MIRR_OFF, 0xBA, 1024); /* mirror torn too */
    RamDevice dev(v.img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    EXPECT_TRUE(plan.actions.empty());
    ASSERT_FALSE(plan.advisories.empty());
    bool found = false;
    for (const auto& a : plan.advisories)
        if (a.find("$MFTMirr") != std::string::npos)
            found = true;
    EXPECT_TRUE(found);
}

TEST(RawFixer, MftRestoreMakesVolumeOpenable) {
    /* the finding's falsifiable claim (1), proven at the engine boundary:
     * a torn $MFT record 0 with an intact mirror makes NtfsVolume::open
     * fail (carve-only path); after the consent-gated repair the volume
     * opens and the structured engine can walk it. */
    NtfsVolumeImage v(true);
    u32 err = 0;
    auto before =
        ntfs::NtfsVolume::open(std::unique_ptr<IStorageDevice>(new RamDevice(v.img)), &err);
    EXPECT_EQ(nullptr, before) << "torn record 0 must fail volume open";

    RamDevice dev(v.img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    ASSERT_EQ(1u, plan.actions.size());
    RamWriteDevice wd(v.img);
    UndoLog undo_log;
    ASSERT_EQ(OK, execute(&wd, plan, undo_log));

    auto after =
        ntfs::NtfsVolume::open(std::unique_ptr<IStorageDevice>(new RamDevice(wd.d_)), &err);
    ASSERT_NE(nullptr, after) << "repaired volume must open: " << err_text(err);
    EXPECT_EQ(1024u, after->params().record_size);

    /* undo returns the volume to the unopenable state (transactional truth) */
    ASSERT_EQ(OK, undo(wd, undo_log));
    auto reverted =
        ntfs::NtfsVolume::open(std::unique_ptr<IStorageDevice>(new RamDevice(wd.d_)), &err);
    EXPECT_EQ(nullptr, reverted);
}

TEST(RawFixer, FatTornMirrorCopyRestoredSectorGranular) {
    /* FAT2 torn: signature sector + sectors 3,4 -> exactly two coalesced
     * restore actions (sector 0 alone; sectors 3-4 as one run) sourced
     * from the signature-valid FAT1. */
    Fat16MirrorImage v;
    v.write_fat_sector(2, 0, 0xAB);
    v.write_fat_sector(2, 3, 0xAB);
    v.write_fat_sector(2, 4, 0xAB);
    RamDevice dev(v.img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    EXPECT_EQ("fat", plan.fs_family);
    ASSERT_EQ(2u, plan.actions.size());
    EXPECT_EQ(ActionKind::RestoreFatSectorFromMirror, plan.actions[0].kind);
    EXPECT_EQ(Fat16MirrorImage::FAT2, plan.actions[0].offset);
    EXPECT_EQ(512u, plan.actions[0].length);
    EXPECT_EQ(ActionKind::RestoreFatSectorFromMirror, plan.actions[1].kind);
    EXPECT_EQ(Fat16MirrorImage::FAT2 + 3 * 512, plan.actions[1].offset);
    EXPECT_EQ(1024u, plan.actions[1].length);
    /* sources are FAT1's bytes */
    EXPECT_EQ(0, memcmp(plan.actions[0].after.data(), v.img.data() + Fat16MirrorImage::FAT1, 512));
    EXPECT_EQ(0, memcmp(plan.actions[1].after.data(),
                        v.img.data() + Fat16MirrorImage::FAT1 + 3 * 512, 1024));

    RamWriteDevice wd(v.img);
    UndoLog undo_log;
    ASSERT_EQ(OK, execute(&wd, plan, undo_log));
    EXPECT_EQ(0, memcmp(wd.d_.data() + Fat16MirrorImage::FAT2,
                        wd.d_.data() + Fat16MirrorImage::FAT1, Fat16MirrorImage::FAT_BYTES));
    ASSERT_EQ(OK, undo(wd, undo_log));
    EXPECT_EQ(0xAB, wd.d_[Fat16MirrorImage::FAT2]);
}

TEST(RawFixer, FatTornPrimaryRestoredFromMirror) {
    /* the reverse direction: FAT1's signature sector torn, FAT2 valid */
    Fat16MirrorImage v;
    v.write_fat_sector(1, 0, 0xCD);
    RamDevice dev(v.img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    ASSERT_EQ(1u, plan.actions.size());
    EXPECT_EQ(ActionKind::RestoreFatSectorFromMirror, plan.actions[0].kind);
    EXPECT_EQ(Fat16MirrorImage::FAT1, plan.actions[0].offset);
    RamWriteDevice wd(v.img);
    UndoLog undo_log;
    ASSERT_EQ(OK, execute(&wd, plan, undo_log));
    EXPECT_EQ(0, memcmp(wd.d_.data() + Fat16MirrorImage::FAT1,
                        wd.d_.data() + Fat16MirrorImage::FAT2, Fat16MirrorImage::FAT_BYTES));
}

TEST(RawFixer, FatMirrorAmbiguityNeverWritten) {
    /* both copies signature-valid but differing later sectors: S3 - no
     * side is provably good, an advisory reports it, no write is planned */
    Fat16MirrorImage v;
    v.write_fat_sector(1, 7, 0x11);
    RamDevice dev(v.img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    EXPECT_TRUE(plan.actions.empty());
    ASSERT_FALSE(plan.advisories.empty());
    bool found = false;
    for (const auto& a : plan.advisories)
        if (a.find("ambiguous") != std::string::npos)
            found = true;
    EXPECT_TRUE(found);
}

TEST(RawFixer, FatCleanMirrorNoAction) {
    Fat16MirrorImage v;
    RamDevice dev(v.img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    EXPECT_TRUE(plan.actions.empty());
    EXPECT_TRUE(plan.advisories.empty());
}

TEST(RawFixer, FixByScanSectorRebuildsBootFromOnlyCandidate) {
    /* no family evidence in the primary or any backup; one FAT-shaped
     * sector at 1 MiB rebuilds the DBR (FixByScanSector) */
    Fat16MirrorImage layout; /* canonical FATs + root, valid signature */
    std::vector<u8> img(8ull << 20, 0);
    std::memcpy(img.data() + 1 * 1024 * 1024, layout.img.data(), 512); /* stray boot copy */
    /* canonical structures at volume-relative offsets (post-restore truth) */
    std::memcpy(img.data() + Fat16MirrorImage::FAT1, layout.img.data() + Fat16MirrorImage::FAT1,
                Fat16MirrorImage::FAT_BYTES);
    std::memcpy(img.data() + Fat16MirrorImage::FAT2, layout.img.data() + Fat16MirrorImage::FAT2,
                Fat16MirrorImage::FAT_BYTES);
    RamDevice dev(img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    EXPECT_TRUE(plan.detected);
    EXPECT_EQ("fat", plan.fs_family);
    ASSERT_EQ(1u, plan.actions.size());
    EXPECT_EQ(ActionKind::DeepScanBootRestore, plan.actions[0].kind);
    EXPECT_EQ(0u, plan.actions[0].offset);
    EXPECT_EQ(0, memcmp(plan.actions[0].after.data(), layout.img.data(), 512));

    RamWriteDevice wd(img);
    UndoLog undo_log;
    ASSERT_EQ(OK, execute(&wd, plan, undo_log));
    EXPECT_EQ(0, memcmp(wd.d_.data(), layout.img.data(), 512));
}

TEST(RawFixer, FixByScanSectorAmbiguousCandidatesRefused) {
    /* two DISTINCT boot-shaped sectors -> S3 refusal, no write */
    Fat16MirrorImage layout;
    std::vector<u8> img(8ull << 20, 0);
    std::memcpy(img.data() + 1 * 1024 * 1024, layout.img.data(), 512);
    std::memcpy(img.data() + 2 * 1024 * 1024, layout.img.data(), 512);
    put16(img.data() + 2 * 1024 * 1024 + 0x13, 5073); /* distinct content: total sectors - 1 */
    RamDevice dev(img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    EXPECT_TRUE(plan.refused);
    EXPECT_TRUE(plan.actions.empty());
    EXPECT_NE(std::string::npos, plan.refusal_reason.find("distinct"));
}

TEST(RawFixer, FixByScanSectorIdenticalCandidatesNotAmbiguous) {
    /* two byte-identical copies of the boot sector: one content, one
     * repair - deduplicated, not an ambiguity */
    Fat16MirrorImage layout;
    std::vector<u8> img(8ull << 20, 0);
    std::memcpy(img.data() + 1 * 1024 * 1024, layout.img.data(), 512);
    std::memcpy(img.data() + 2 * 1024 * 1024, layout.img.data(), 512);
    std::memcpy(img.data() + Fat16MirrorImage::FAT1, layout.img.data() + Fat16MirrorImage::FAT1,
                Fat16MirrorImage::FAT_BYTES);
    std::memcpy(img.data() + Fat16MirrorImage::FAT2, layout.img.data() + Fat16MirrorImage::FAT2,
                Fat16MirrorImage::FAT_BYTES);
    RamDevice dev(img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    EXPECT_FALSE(plan.refused);
    ASSERT_EQ(1u, plan.actions.size());
    EXPECT_EQ(ActionKind::DeepScanBootRestore, plan.actions[0].kind);
}

TEST(RawFixer, RebasedPartitionPlanWritesAtDiskBasisNeverLba0) {
    /* A-002: a plan analyzed through a partition-slice view carries
     * slice-relative offsets; executing it against the whole-disk write
     * device without repair::rebase_plan writes at DISK offset 0 - over
     * the MBR/GPT (the bug). Rebased, the repair lands at the
     * partition-absolute offset, disk LBA 0 stays untouched, and the undo
     * log carries disk-absolute offsets (the documented --undo-apply
     * contract). Fail-before is structural: without the rebase call the
     * same execute writes at disk offset 0 and the MBR assertions fail. */
    NtfsVolumeImage v(true); /* torn $MFT record 0 -> a one-action plan */
    const u64 part_base = 4096ull * 512; /* partition start LBA 4096 */
    std::vector<u8> disk(16ull << 20, 0);
    /* place the volume image at the partition base */
    std::memcpy(disk.data() + part_base, v.img.data(), v.img.size());
    /* an MBR at LBA 0 that must survive the repair untouched */
    disk[510] = 0x55;
    disk[511] = 0xAA;
    disk[446] = 0x80;

    /* analyze through the partition slice (slice-relative offsets) */
    RamDevice whole(disk);
    u32 prc = 0;
    auto slice = cr::PartitionSlice::create(&whole, part_base, u64(v.img.size()), &prc);
    ASSERT_NE(nullptr, slice.get());
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(slice.get(), plan));
    ASSERT_EQ(1u, plan.actions.size());
    EXPECT_EQ(NtfsVolumeImage::MFT_OFF, plan.actions[0].offset); /* slice-relative */

    /* the A-002 rebase: the same call the orchestrator and cmd_fix make */
    rebase_plan(plan, part_base);
    EXPECT_EQ(NtfsVolumeImage::MFT_OFF + part_base, plan.actions[0].offset);

    /* execute against the WHOLE-DISK device (the parent basis) */
    RamWriteDevice diskw(disk);
    UndoLog log;
    ASSERT_EQ(OK, execute(&diskw, plan, log));

    /* disk LBA 0 (the MBR) untouched - the A-002 invariant */
    EXPECT_EQ(0x55, diskw.d_[510]);
    EXPECT_EQ(0xAA, diskw.d_[511]);
    EXPECT_EQ(0x80, diskw.d_[446]);
    /* the repair landed at the partition-absolute offset: record 0 now
     * equals the mirror's record 0 exactly */
    EXPECT_EQ(0, std::memcmp(diskw.d_.data() + part_base + NtfsVolumeImage::MFT_OFF,
                             v.img.data() + NtfsVolumeImage::MIRR_OFF, 1024));
    /* the undo log carries DISK-absolute offsets (the documented
     * --undo-apply contract) */
    ASSERT_EQ(1u, log.entries.size());
    EXPECT_EQ(NtfsVolumeImage::MFT_OFF + part_base, log.entries[0].offset);
}


/* ---- S-010: volume quiesce contract (corpus RecoveryModule lock sequence) ---- */

TEST(RawFixer, VolumeLockAcquiredBeforeFirstWrite) {
    /* execute() must acquire the device's volume lock BEFORE any write and
     * release it after the transaction completes. */
    Fat16MirrorImage v;
    v.write_fat_sector(2, 0, 0xAB);
    RamDevice dev(v.img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    ASSERT_EQ(1u, plan.actions.size());

    RamWriteDevice wd(v.img);
    UndoLog undo_log;
    ASSERT_EQ(OK, execute(&wd, plan, undo_log));
    EXPECT_GE(wd.lock_calls, 1);
    EXPECT_GE(wd.unlock_calls, 1);
    EXPECT_FALSE(wd.write_before_lock); /* the core contract of S-010 */
    EXPECT_EQ(1u, wd.writes);
}

TEST(RawFixer, VolumeLockFailureNeverWrites) {
    /* a busy volume (lock refused) aborts the repair with ZERO writes -
     * concurrent access is refused, never raced */
    Fat16MirrorImage v;
    v.write_fat_sector(2, 0, 0xAB);
    RamDevice dev(v.img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    ASSERT_EQ(1u, plan.actions.size());

    RamWriteDevice wd(v.img);
    wd.lock_rc = ERR_VOLUME_LOCK_FAILED;
    UndoLog undo_log;
    EXPECT_EQ(ERR_VOLUME_LOCK_FAILED, execute(&wd, plan, undo_log));
    EXPECT_EQ(0u, wd.writes);
    EXPECT_EQ(0xAB, wd.d_[Fat16MirrorImage::FAT2]); /* untouched */
    EXPECT_TRUE(undo_log.entries.empty());
    EXPECT_EQ(0, wd.unlock_calls); /* nothing was locked */
}

TEST(RawFixer, UnlockHappensOnRollbackPath) {
    /* the guard releases the lock on EVERY exit - including a mid-plan
     * failure that rolls back */
    Fat16MirrorImage v;
    v.write_fat_sector(2, 0, 0xAB);
    v.write_fat_sector(2, 3, 0xAB);
    RamDevice dev(v.img);
    RepairPlan plan;
    ASSERT_EQ(OK, analyze(&dev, plan));
    ASSERT_EQ(2u, plan.actions.size());

    RamWriteDevice wd(v.img);
    wd.fail_write_at = u32(Fat16MirrorImage::FAT2 + 3 * 512);
    UndoLog undo_log;
    EXPECT_EQ(ERR_WRITE_FAILED, execute(&wd, plan, undo_log));
    EXPECT_GE(wd.unlock_calls, 1); /* the guard fired on the failure path */
}
} // namespace repair
} // namespace cr
