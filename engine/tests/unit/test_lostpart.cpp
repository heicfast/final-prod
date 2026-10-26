/* Lost-partition discovery tests (R3). */
#include <gtest/gtest.h>
#include "cr/lost/cr_lostpart.h"
#include "cr/fat/cr_fat_engine.h"
#include "cr/storage/cr_device.h"
#include "cr/table/cr_table_parse.h"
#include <cstring>
#include <vector>

using namespace cr;
using namespace cr::lost;

namespace {

class MemDevice final : public IStorageDevice {
public:
    explicit MemDevice(std::vector<u8> b) : data_(std::move(b)) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = data_.size() / 512;
        geom_.total_bytes = geom_.sector_count * 512;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }

private:
    std::vector<u8> data_;
    Geometry geom_;
};

void put16(std::vector<u8>& v, size_t at, u16 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
}
void put32v(std::vector<u8>& v, size_t at, u32 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
    v[at + 2] = u8(x >> 16);
    v[at + 3] = u8(x >> 24);
}
void put64(std::vector<u8>& v, size_t at, u64 x) {
    for (int i = 0; i < 8; ++i)
        v[at + i] = u8(x >> (8 * i));
}

/* a plausible NTFS boot sector at `lba` + a live $MFT record at mft_lcn */
void plant_ntfs(std::vector<u8>& img, u64 lba, u64 total_sectors, u64 mft_lcn) {
    u8* b = img.data() + lba * 512;
    std::memcpy(b + 3, "NTFS    ", 8);
    put16(img, lba * 512 + 0x0B, 512);
    b[0x0D] = 1; /* spc */
    b[0x15] = 0xF8;
    put64(img, lba * 512 + 0x28, total_sectors);
    put64(img, lba * 512 + 0x30, mft_lcn);
    put64(img, lba * 512 + 0x38, mft_lcn + 8);
    b[0x40] = 2; /* clusters/record: 1024B */
    b[0x44] = 2;
    b[510] = 0x55;
    b[511] = 0xAA;
    /* fake $MFT record at lba + mft_lcn (volume-relative): 'FILE' + USA */
    const u64 rec_off = (lba + mft_lcn) * 512;
    b = img.data() + rec_off;
    put32v(img, rec_off + 0, 0x454C4946); /* 'FILE' */
    put16(img, rec_off + 4, 0x30);        /* USA offset */
    put16(img, rec_off + 6, 3);           /* USA size: 2+1 */
    put16(img, rec_off + 8, 1);           /* sequence */
    put16(img, rec_off + 0x14, 0x38);     /* attribute offset */
    put16(img, rec_off + 0x18, 1);        /* in-use */
    put32v(img, rec_off + 0x1C, 1024);
    put32v(img, rec_off + 0x20, 1024);
    put32v(img, rec_off + 0x2C, u32(0));
    /* USA slots: USN, orig0, orig1 */
    put16(img, rec_off + 0x30, 0x1234);
    put16(img, rec_off + 0x32, 0xAAAA);
    put16(img, rec_off + 0x34, 0xBBBB);
    /* tails carry the USN */
    img[rec_off + 510] = 0x34;
    img[rec_off + 511] = 0x12;
    img[rec_off + 1022] = 0x34;
    img[rec_off + 1023] = 0x12;
}

void plant_exfat_boot(std::vector<u8>& img, u64 lba, u32 volume_length, u32 cluster_count) {
    u8* b = img.data() + lba * 512;
    b[0] = 0xEB;
    b[1] = 0x76;
    b[2] = 0x90;
    std::memcpy(b + 3, "EXFAT   ", 8);
    put64(img, lba * 512 + 72, volume_length);
    put32v(img, lba * 512 + 80, 24);
    put32v(img, lba * 512 + 84, 1);
    put32v(img, lba * 512 + 88, 25);
    put32v(img, lba * 512 + 92, cluster_count);
    put32v(img, lba * 512 + 96, 2);
    b[108] = 9;
    b[510] = 0x55;
    b[511] = 0xAA;
}

std::vector<u8> build_disk() {
    std::vector<u8> img(u64(8192) * 512, 0);
    /* live MBR: one NTFS-typed partition at 2048 len 1024 (no boot sector
     * content - it is the KNOWN table, not a discovery target) */
    u8* mbr = img.data();
    mbr[446 + 4] = 0x07;
    put32v(img, 446 + 8, 2048);
    put32v(img, 446 + 12, 1024);
    mbr[510] = 0x55;
    mbr[511] = 0xAA;

    plant_ntfs(img, 1024, 512, 100);        /* mft_lcn inside the volume */
    plant_exfat_boot(img, 4096, 2048, 100); /* spec-min size, own region */
    /* noise: 55AA with garbage BPB at sector 500 */
    img[500 * 512 + 510] = 0x55;
    img[500 * 512 + 511] = 0xAA;
    img[500 * 512 + 0x0B] = 0x11; /* nonsense bps */
    return img;
}

} // namespace

TEST(LostDiscovery, FindsScoresAndRejectsNoise) {
    auto img = build_disk();
    MemDevice dev(std::move(img));

    /* live table for overlap classification */
    table::DiskTable t;
    ASSERT_EQ(table::parse_disk_table(&dev, t), OK);
    ASSERT_EQ(t.partitions.size(), 1u);
    EXPECT_EQ(t.partitions[0].start_lba, 2048u);

    DiscoveryResult r;
    u32 rc = discover(&dev, t.partitions, 1, nullptr, nullptr, nullptr, r);
    ASSERT_EQ(rc, OK) << err_text(rc);
    EXPECT_GE(r.sectors_scanned, 4096u);

    /* locate candidates */
    const Candidate *ntfs_c = nullptr, *exfat_c = nullptr, *noise = nullptr;
    for (const auto& c : r.candidates) {
        if (c.start_lba == 1024)
            ntfs_c = &c;
        else if (c.start_lba == 4096)
            exfat_c = &c;
        else if (c.start_lba == 500)
            noise = &c;
    }
    ASSERT_NE(ntfs_c, nullptr);
    ASSERT_NE(exfat_c, nullptr);
    EXPECT_EQ(noise, nullptr); /* incoherent BPB rejected */

    EXPECT_EQ(ntfs_c->fs, CandidateFs::Ntfs);
    EXPECT_EQ(ntfs_c->structural_points, 70); /* 15 boot + 25 bpb + 30 mft */
    EXPECT_EQ(ntfs_c->score, 1000);           /* M-017: best content count this scan
                  (60: no files, NTFS bonus) - ln-normalized to the top */
    EXPECT_EQ(ntfs_c->sector_count, 512u);
    EXPECT_EQ(ntfs_c->start_lba + ntfs_c->sector_count, 1536u); /* no tail overlap
        with the live partition at 2048 -> no Overlaps penalty in this case */

    EXPECT_EQ(exfat_c->fs, CandidateFs::Exfat);
    EXPECT_EQ(exfat_c->structural_points, 35); /* 15 + 20 */
    /* M-017: count 0 (no content, no bonus pinned) -> 1/ln(60) = 244 */
    EXPECT_EQ(exfat_c->score, 244);
    EXPECT_EQ(exfat_c->sector_count, 2048u);

    /* sorted desc */
    ASSERT_GE(r.candidates.size(), 2u);
    EXPECT_GE(r.candidates[0].score, r.candidates[1].score);

    /* materialize + read through the virtual slice */
    auto slice = materialize(&dev, *ntfs_c);
    ASSERT_NE(slice, nullptr);
    u8 buf[9];
    EXPECT_EQ(slice->read_at(3, buf, 8), OK);
    buf[8] = 0;
    EXPECT_EQ(std::memcmp(buf, "NTFS    ", 8), 0);
}

TEST(LostDiscovery, OverlapClassification) {
    table::PartitionInfo live;
    live.start_lba = 1000;
    live.sector_count = 500;
    live.used = true;
    std::vector<table::PartitionInfo> known{live};
    EXPECT_EQ(classify_overlap(0, 100, known), Overlap::None);
    EXPECT_EQ(classify_overlap(1100, 50, known), Overlap::Inside);
    EXPECT_EQ(classify_overlap(900, 200, known), Overlap::Overlaps);
    EXPECT_EQ(classify_overlap(900, 800, known), Overlap::Contains);
}

/* ---- GAP-S6-6: TWO deleted NTFS partitions must BOTH surface ----
 *
 * Bench run 40073093997 delete_partition: the deep tree carried only
 * 'Candidate @ 32768' (partition A) while the og found BOTH deleted
 * partitions. Registered suspect: the nested-same-fs dedup when the
 * outer candidate's size came from the DEVICE-TAIL FALLBACK (a probe
 * failure path) - a fallback-sized extent spans to disk end and
 * swallows every later peer. */

TEST(LostDiscovery, TwoHealthyDeletedPartitionsBothSurface) {
    /* the bench shape: two ~equal NTFS partitions at unequal offsets,
     * both boot sectors intact (GPT entries removed - data untouched). */
    std::vector<u8> img(u64(16384) * 512, 0); /* 8 MiB disk */
    plant_ntfs(img, 32768 / 8, 4096, 64);     /* A: LBA 4096, 2 MiB */
    plant_ntfs(img, 12288, 4096, 64);         /* B: LBA 12288, 2 MiB */
    MemDevice dev(std::move(img));
    DiscoveryResult r;
    ASSERT_EQ(discover(&dev, {}, 1, nullptr, nullptr, nullptr, r), OK);
    size_t ntfs = 0;
    for (const auto& c : r.candidates)
        if (c.fs == CandidateFs::Ntfs)
            ++ntfs;
    EXPECT_EQ(ntfs, 2u) << "both healthy deleted NTFS partitions must surface";
}

TEST(LostDiscovery, DamagedBpbFallbackMustNotSwallowPeer) {
    /* A's BPB is damaged (fallback size = device tail - spans to the end);
     * B (healthy) starts inside that envelope. The nested-same-fs dedup
     * must NOT treat a fallback-sized extent as containment evidence. */
    std::vector<u8> img(u64(16384) * 512, 0);
    plant_ntfs(img, 4096, 4096, 64);  /* A: healthy shape first */
    plant_ntfs(img, 12288, 4096, 64); /* B: healthy, later offset */
    /* damage A's BPB AFTER planting (total-sectors field destroyed -> the
     * probe falls back to the device-end size) */
    put64(img, 4096 * 512 + 0x28, 0);
    MemDevice dev(std::move(img));
    DiscoveryResult r;
    ASSERT_EQ(discover(&dev, {}, 1, nullptr, nullptr, nullptr, r), OK);
    size_t ntfs = 0;
    u64 b_found = 0;
    for (const auto& c : r.candidates)
        if (c.fs == CandidateFs::Ntfs) {
            ++ntfs;
            if (c.start_lba == 12288)
                b_found = 1;
        }
    EXPECT_EQ(ntfs, 2u) << "the fallback-sized A must not dedup away B";
    EXPECT_EQ(b_found, 1u) << "B (healthy probe, inside A's fallback envelope) must surface";
}

/* ---- S-006: superblock/volume-header probe families --------------------- */

void plant_ext4_sb(std::vector<u8>& img, u64 lba, u32 blocks, u32 bpg, u32 ipg, u32 inodes,
                   u16 block_group_nr, u32 inode_table_block) {
    /* ext2/3/4 superblock at lba+1024 (block size 4096: first_data_block=0,
     * log_bs=2; GDT at partition block 1 = byte 4096) */
    const u64 sb = lba * 512 + 1024;
    put32v(img, sb + 0x00, inodes);        /* s_inodes_count */
    put32v(img, sb + 0x04, blocks);        /* s_blocks_count (lo) */
    put32v(img, sb + 0x14, 0);             /* s_first_data_block (bs=4096) */
    put32v(img, sb + 0x18, 2);             /* s_log_block_size = 2 -> 4096 */
    put32v(img, sb + 0x20, bpg);           /* s_blocks_per_group */
    put32v(img, sb + 0x28, ipg);           /* s_inodes_per_group */
    put16(img, sb + 0x38, 0xEF53);         /* s_magic */
    put32v(img, sb + 0x4C, 1);             /* s_rev_level = dynamic */
    put32v(img, sb + 0x54, 11);            /* s_first_ino (>= 3 gate) */
    put16(img, sb + 0x5A, block_group_nr); /* s_block_group_nr */
    put16(img, sb + 0x58, 256);            /* s_inode_size */
    /* group descriptor table: block 1 (byte 4096 of the partition);
     * bg_block_bitmap_lo@0, bg_inode_bitmap_lo@4, bg_inode_table_lo@8 */
    const u64 gdt = lba * 512 + 4096;
    put32v(img, gdt + 0, 4); /* block bitmap block 4 */
    put32v(img, gdt + 4, 8); /* inode bitmap block 8 */
    put32v(img, gdt + 8, inode_table_block);
}

void plant_hfsplus_vh(std::vector<u8>& img, u64 lba, u32 block_size, u32 total_blocks) {
    /* HFS+ volume header at lba+1024, big-endian fields */
    const u64 vh = lba * 512 + 1024;
    img[vh + 0] = 0x48;
    img[vh + 1] = 0x2B; /* "H+" */
    img[vh + 2] = 0;
    img[vh + 3] = 4; /* version 4 */
    /* block_size (BE u32) @0x28, total_blocks (BE u32) @0x2C; forks at
     * 0x70/0xC0 stay zeroed (total_blocks 0 <= total) */
    img[vh + 0x28] = u8(block_size >> 24);
    img[vh + 0x29] = u8(block_size >> 16);
    img[vh + 0x2A] = u8(block_size >> 8);
    img[vh + 0x2B] = u8(block_size);
    img[vh + 0x2C] = u8(total_blocks >> 24);
    img[vh + 0x2D] = u8(total_blocks >> 16);
    img[vh + 0x2E] = u8(total_blocks >> 8);
    img[vh + 0x2F] = u8(total_blocks);
}

void plant_btrfs_sb(std::vector<u8>& img, u64 lba, u32 sectorsize, u32 nodesize) {
    /* Btrfs primary superblock at partition+64 KiB */
    const u64 sb = lba * 512 + 0x10000;
    std::memcpy(img.data() + sb + 0x40, "_BHRfS_M", 8); /* magic */
    put32v(img, sb + 0x90, sectorsize);
    put32v(img, sb + 0x94, nodesize);
    put32v(img, sb + 0xA0, 0); /* sys_array_size 0: valid anchor */
}

void plant_zfs_label(std::vector<u8>& img, u64 lba, u64 txg, u64 spa_version) {
    /* ZFS vdev label-0 uberblock ring head at partition+128 KiB */
    const u64 ub = lba * 512 + 0x20000;
    put32v(img, ub + 0, 0x00BAB10C); /* magic (LE u64 low word) */
    put32v(img, ub + 4, 0);
    put64(img, ub + 8, spa_version);
    put64(img, ub + 16, txg);
}

void plant_refs_boot(std::vector<u8>& img, u64 lba, bool checkpointed) {
    /* ReFS DBR (OEM "ReFS" + 55AA) + one metadata session page in the
     * first MiB, at a 512-aligned offset */
    u8* b = img.data() + lba * 512;
    b[0] = 0xEB;
    b[1] = 0x52;
    b[2] = 0x90;
    std::memcpy(b + 3, "ReFS", 4);
    b[510] = 0x55;
    b[511] = 0xAA;
    const char* page = checkpointed ? "CHKPSUPBMSB+ReFSFSRS" : "SUPBMSB+ReFSFSRS";
    std::memcpy(img.data() + lba * 512 + 0x1000, page, checkpointed ? size_t(20) : size_t(16));
}

TEST(LostDiscovery, Ext4SuperblockAnchorsDeletedPartition) {
    /* S-006 falsifiable claim: a deleted ext4 partition (superblock
     * remnant intact, no partition-table entry) surfaces as a candidate in
     * the og's deep scan (CExtXBlockParser) - and now in ours. */
    std::vector<u8> img(u64(32768) * 512, 0);
    /* 2048 4-KiB blocks, one block group (bpg 2048; the engine validator
     * requires blocks/bpg >= 1), inodes 2048 / ipg 2048 (group formulas
     * agree), first_ino 11, block_group_nr 0 */
    plant_ext4_sb(img, 6144, 2048 /* blocks */, 2048 /* bpg */, 2048 /* ipg */, 2048 /* inodes */,
                  0 /* block_group_nr */, 100);
    MemDevice dev(std::move(img));
    DiscoveryResult r;
    ASSERT_EQ(discover(&dev, {}, 1, nullptr, nullptr, nullptr, r), OK);
    const Candidate* ext_c = nullptr;
    for (const auto& c : r.candidates)
        if (c.start_lba == 6144)
            ext_c = &c;
    ASSERT_NE(ext_c, nullptr);
    EXPECT_EQ(ext_c->fs, CandidateFs::Ext2);
    EXPECT_GE(ext_c->score, 70); /* 15 magic + 25 validation + 30 GDT probe */
    EXPECT_EQ(ext_c->sector_count, 2048u * 4096u / 512u); /* SB-derived size */
    EXPECT_FALSE(ext_c->size_from_device_end);
    /* materialize + the orchestrator's probe_family sees the same ext4 */
    auto slice = materialize(&dev, *ext_c);
    ASSERT_NE(slice, nullptr);
    u8 m[2] = {};
    EXPECT_EQ(slice->read_at(1024 + 0x38, m, 2), OK);
    EXPECT_EQ(m[0], 0x53);
    EXPECT_EQ(m[1], 0xEF);
}

TEST(LostDiscovery, Ext4BlockGroupCopyNotACandidateStart) {
    /* the og's CExtXBlockParser gate: only block-group-0 superblocks anchor
     * a partition start (s_block_group_nr == 0). A later SB copy (every
     * ext4 carries them at block-group boundaries) must NOT surface. */
    std::vector<u8> img(u64(16384) * 512, 0);
    /* a copy with block_group_nr = 5 and its own (self-consistent) geometry */
    plant_ext4_sb(img, 6144, 8192, 32768, 2048, 4096, 5, 900);
    MemDevice dev(std::move(img));
    DiscoveryResult r;
    ASSERT_EQ(discover(&dev, {}, 1, nullptr, nullptr, nullptr, r), OK);
    for (const auto& c : r.candidates)
        EXPECT_NE(c.start_lba, 6144u) << "block-group copy must not anchor a candidate";
}

TEST(LostDiscovery, HfsPlusVolumeHeaderAnchorsDeletedPartition) {
    std::vector<u8> img(u64(16384) * 512, 0);
    plant_hfsplus_vh(img, 6144, 4096, 1024); /* 1024 4-KiB blocks = 4 MiB */
    MemDevice dev(std::move(img));
    DiscoveryResult r;
    ASSERT_EQ(discover(&dev, {}, 1, nullptr, nullptr, nullptr, r), OK);
    const Candidate* h = nullptr;
    for (const auto& c : r.candidates)
        if (c.start_lba == 6144)
            h = &c;
    ASSERT_NE(h, nullptr);
    EXPECT_EQ(h->fs, CandidateFs::HfsPlus);
    EXPECT_GE(h->score, 40); /* 35 vh + 5 geometry */
    EXPECT_EQ(h->sector_count, 1024u * 4096u / 512u);
}

TEST(LostDiscovery, BtrfsSuperblockAnchorsDeletedPartition) {
    std::vector<u8> img(u64(16384) * 512, 0);
    plant_btrfs_sb(img, 6144, 4096, 4096);
    MemDevice dev(std::move(img));
    DiscoveryResult r;
    ASSERT_EQ(discover(&dev, {}, 1, nullptr, nullptr, nullptr, r), OK);
    const Candidate* b = nullptr;
    for (const auto& c : r.candidates)
        if (c.start_lba == 6144)
            b = &c;
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(b->fs, CandidateFs::Btrfs);
    EXPECT_GE(b->score, 35);
}

TEST(LostDiscovery, ZfsLabelAnchorsDeletedPartition) {
    std::vector<u8> img(u64(16384) * 512, 0);
    plant_zfs_label(img, 6144, 5 /* txg */, 5000 /* spa_version */);
    MemDevice dev(std::move(img));
    DiscoveryResult r;
    ASSERT_EQ(discover(&dev, {}, 1, nullptr, nullptr, nullptr, r), OK);
    const Candidate* z = nullptr;
    for (const auto& c : r.candidates)
        if (c.start_lba == 6144)
            z = &c;
    ASSERT_NE(z, nullptr);
    EXPECT_EQ(z->fs, CandidateFs::Zfs);
    EXPECT_GE(z->score, 35);              /* 25 magic + 10 txg liveness */
    EXPECT_TRUE(z->size_from_device_end); /* flagged fallback (honest) */
}

TEST(LostDiscovery, RefsBootAnchorsDeletedPartition) {
    /* the ReFS DBR (OEM "ReFS" + 55AA) used to fall into the FAT catch-all
     * and get dropped as noise; the page-signature window probe now takes
     * it before the catch-all. */
    std::vector<u8> img(u64(16384) * 512, 0);
    plant_refs_boot(img, 6144, true /* checkpointed */);
    MemDevice dev(std::move(img));
    DiscoveryResult r;
    ASSERT_EQ(discover(&dev, {}, 1, nullptr, nullptr, nullptr, r), OK);
    const Candidate* rf = nullptr;
    for (const auto& c : r.candidates)
        if (c.start_lba == 6144)
            rf = &c;
    ASSERT_NE(rf, nullptr);
    EXPECT_EQ(rf->fs, CandidateFs::Refs);
    EXPECT_GE(rf->score, 40);              /* 35 pages + 5 checkpointed */
    EXPECT_TRUE(rf->size_from_device_end); /* flagged fallback (honest) */
}

TEST(LostDiscovery, SuperblockNoiseRejected) {
    /* wrong magics / incoherent geometry surface no candidates */
    std::vector<u8> img(u64(16384) * 512, 0);
    /* 0xEF53 at a place with no coherent superblock around it */
    put16(img, 8192 * 512 + 1024 + 0x38, 0xEF53);
    /* "H+" signature with garbage geometry (block_size 0) */
    img[9216 * 512 + 1024 + 0] = 0x48;
    img[9216 * 512 + 1024 + 1] = 0x2B;
    /* btrfs magic with invalid nodesize (not pow2) */
    std::memcpy(img.data() + 10240 * 512 + 0x10040, "_BHRfS_M", 8);
    put32v(img, 10240 * 512 + 0x10090, 4096);
    put32v(img, 10240 * 512 + 0x10094, 3000);
    /* ZFS magic without a txg is still a (weak) candidate - but magic-less
     * ZFS geometry must not surface; test the ext group-formula mismatch:
     * a "superblock" whose block-derived and inode-derived group counts
     * disagree (the og's FUN_180002880 cross-check) */
    plant_ext4_sb(img, 11264, 4096, 32768, 2048, 2048, 0, 100);
    put32v(img, 11264 * 512 + 1024 + 0x28, 1024); /* ipg now mismatches: gb=1 gi=2 */
    MemDevice dev(std::move(img));
    DiscoveryResult r;
    ASSERT_EQ(discover(&dev, {}, 1, nullptr, nullptr, nullptr, r), OK);
    EXPECT_TRUE(r.candidates.empty());
}

/* ---- S-019: BitLocker (FVE) lost-partition probe ------------------------ */

namespace {

void plant_fve_boot(std::vector<u8>& img, u64 lba, const char* version) {
    /* BitLocker VBR shape: 3-byte jump, OEM "-FVE-FS-", printable version
     * string ("2013.0.0." / "2.0.1.0" style), 0x55AA signature. Real FVE
     * VBRs pad the unused region with 0xFF - mirrored here. */
    u8* b = img.data() + lba * 512;
    b[0] = 0xEB;
    b[1] = 0x52;
    b[2] = 0x90;
    std::memcpy(b + 3, "-FVE-FS-", 8);
    std::memset(b + 0x0B, 0xFF, 0x1F5 - 0x0B);
    const size_t vlen = std::strlen(version);
    std::memcpy(b + 0x0B, version, vlen < 16 ? vlen : 16);
    b[510] = 0x55;
    b[511] = 0xAA;
}

} // namespace

TEST(LostDiscovery, LostBitlockerPartitionDiscovered) {
    /* S-019 falsifiable claim: a deleted BitLocker-encrypted partition
     * (FVE boot sector intact at the old partition start, outside the
     * current table) yields a candidate in the og's deep scan (type 0xB,
     * BitlockerParser) - and now in ours. Pre-S-019 the FVE sector died in
     * the FAT catch-all (fat::validate_boot rejects the BPB-less VBR). */
    std::vector<u8> img(u64(16384) * 512, 0);
    plant_fve_boot(img, 6144, "2013.0.0.");
    MemDevice dev(std::move(img));
    DiscoveryResult r;
    ASSERT_EQ(discover(&dev, {}, 1, nullptr, nullptr, nullptr, r), OK);
    const Candidate* fve = nullptr;
    for (const auto& c : r.candidates)
        if (c.start_lba == 6144)
            fve = &c;
    ASSERT_NE(fve, nullptr) << "the lost BitLocker partition must surface";
    EXPECT_EQ(fve->fs, CandidateFs::BitLocker);
    EXPECT_GE(fve->score, 40);              /* 15 boot-sig + 25 fve-shape */
    EXPECT_TRUE(fve->size_from_device_end); /* geometry corpus-undecoded: marked fallback */
    EXPECT_GT(fve->sector_count, 0u);
    /* evidence tags carry the og-gate + shape breakdown */
    bool has_boot_sig = false, has_shape = false;
    for (const auto& e : fve->evidence) {
        if (std::strcmp(e.tag, "boot-sig") == 0)
            has_boot_sig = true;
        if (std::strcmp(e.tag, "fve-shape") == 0)
            has_shape = true;
    }
    EXPECT_TRUE(has_boot_sig);
    EXPECT_TRUE(has_shape);

    /* the orchestrator's probe_family recognizes the same OEM and routes
     * to the BitLocker family (never the ext4-magic accident) */
    auto slice = materialize(&dev, *fve);
    ASSERT_NE(slice, nullptr);
    u8 oem[8] = {};
    ASSERT_EQ(slice->read_at(3, oem, 8), OK);
    EXPECT_EQ(0, std::memcmp(oem, "-FVE-FS-", 8));
}

TEST(LostDiscovery, LostBitlockerOemNoise) {
    /* the OEM label alone is not enough: without the 0x55AA boot signature
     * the classifier ladder never fires (the og's classifier runs inside
     * the boot-sector family pass). Random OEM + 55AA but garbage version
     * region still surfaces ONLY via the boot-sig points (the og's gate
     * has no shape requirement) - graded, not rejected. */
    std::vector<u8> img(u64(16384) * 512, 0);
    /* noise 1: "-FVE-FS-" without 55AA -> no candidate at that LBA */
    u8* n1 = img.data() + 8192 * 512;
    std::memcpy(n1 + 3, "-FVE-FS-", 8);
    /* noise 2: "-FVE-FS-" + 55AA but binary garbage version region ->
     * candidate (og gate) with ONLY boot-sig evidence */
    u8* n2 = img.data() + 9216 * 512;
    std::memcpy(n2 + 3, "-FVE-FS-", 8);
    n2[0x0B] = 0x01;
    n2[0x0C] = 0xA2; /* binary bytes: not digits/dots */
    n2[0x0D] = 0x7F;
    n2[510] = 0x55;
    n2[511] = 0xAA;
    MemDevice dev(std::move(img));
    DiscoveryResult r;
    ASSERT_EQ(discover(&dev, {}, 1, nullptr, nullptr, nullptr, r), OK);
    const Candidate* c8192 = nullptr;
    const Candidate* c9216 = nullptr;
    for (const auto& c : r.candidates) {
        if (c.start_lba == 8192)
            c8192 = &c;
        if (c.start_lba == 9216)
            c9216 = &c;
    }
    EXPECT_EQ(c8192, nullptr) << "no 55AA: the classifier must not fire";
    ASSERT_NE(c9216, nullptr);
    EXPECT_EQ(c9216->fs, CandidateFs::BitLocker);
    EXPECT_EQ(c9216->structural_points, 15); /* boot-sig only (M-017: the
        structural points are the tie-break key; the score is the
        content model - a lone BitLocker candidate is best-of-scan) */
}

TEST(LostDiscovery, LostBitlockerInsideExtendedRegionStillFound) {
    /* the deep scan is table-independent: an FVE boot sector that happens
     * to sit inside the extended-partition region (EBR chain area) is
     * still discovered - the og's CompleteSearch generator scans device
     * extents, not table entries. */
    std::vector<u8> img(u64(16384) * 512, 0);
    /* live MBR: extended partition [2048, 4096) with an EBR at 2048 */
    u8* mbr = img.data();
    mbr[446 + 4] = 0x05;
    put32v(img, 446 + 8, 2048);
    put32v(img, 446 + 12, 2048);
    mbr[510] = 0x55;
    mbr[511] = 0xAA;
    u8* ebr = img.data() + 2048 * 512;
    ebr[510] = 0x55;
    ebr[511] = 0xAA;
    /* the FVE volume starts INSIDE the extended region (a deleted logical
     * volume that was BitLocker-encrypted) */
    plant_fve_boot(img, 6144, "2.0.1.0");
    MemDevice dev(std::move(img));
    std::vector<table::PartitionInfo> known;
    table::DiskTable t;
    ASSERT_EQ(table::parse_disk_table(&dev, t), OK);
    for (const auto& p : t.partitions)
        known.push_back(p);
    DiscoveryResult r;
    ASSERT_EQ(discover(&dev, known, 1, nullptr, nullptr, nullptr, r), OK);
    const Candidate* fve = nullptr;
    for (const auto& c : r.candidates)
        if (c.start_lba == 6144)
            fve = &c;
    ASSERT_NE(fve, nullptr) << "table-independent discovery must reach the FVE sector";
    EXPECT_EQ(fve->fs, CandidateFs::BitLocker);
}

/* ---- N-034 (diff-to-fix session): the og's deep-scan FAT ladder gates -- */

namespace {

/* Plant a FAT boot sector with the given geometry knobs. */
void plant_fat_boot(std::vector<u8>& img, u64 lba, int spc, int num_fats, u16 root_entries,
                    u32 total32, u16 spf16, u32 spf32, bool fat32_string) {
    u8* b = img.data() + lba * 512;
    b[0] = 0xEB;
    b[1] = 0x3C;
    b[2] = 0x90;
    std::memcpy(b + 3, "MSDOS5.0", 8);
    put16(img, lba * 512 + 0x0B, 512);
    b[0x0D] = u8(spc);
    put16(img, lba * 512 + 0x0E, 2);
    b[0x10] = u8(num_fats);
    put16(img, lba * 512 + 0x11, root_entries);
    put16(img, lba * 512 + 0x13, 0); /* total16 unused */
    b[0x15] = 0xF8;
    put16(img, lba * 512 + 0x16, spf16);
    put32v(img, lba * 512 + 0x20, total32);
    put32v(img, lba * 512 + 0x24, spf32);
    if (fat32_string)
        std::memcpy(b + 0x52, "FAT32   ", 8);
    b[510] = 0x55;
    b[511] = 0xAA;
}

} // namespace

TEST(LostFatGates, Spc256Rejected) {
    /* N-034: SPC 256 is outside the og deep-gate set {1..128}; the old
     * core's any-pow2 acceptance produced a candidate every og gate
     * rejects. */
    std::vector<u8> img(1024 * 512, 0);
    plant_fat_boot(img, 16, 256, 2, 512, 900, 20, 0, false);
    EXPECT_EQ(fat::lost_boot_flavor(img.data() + 16 * 512), 0);
    /* in-set SPC 128 on the same shape passes (6 data clusters -> the
     * small bracket's FAT12 gate) */
    plant_fat_boot(img, 16, 128, 2, 512, 900, 20, 0, false);
    EXPECT_EQ(fat::lost_boot_flavor(img.data() + 16 * 512), 1);
}

TEST(LostFatGates, NumFatsZeroRejectedThreeAcceptedByDeepGates) {
    /* N-034: the og's DEEP gates only require numFATs != 0
     * (FUN_1800048f0/4a40/4b70) - a triple-FAT boot IS a lost candidate
     * on the og's deep scan (the {1,2} restriction lives at ENGINE OPEN,
     * FUN_18002f970 :38049, where such a volume is declined). Zero
     * FATs is rejected everywhere. */
    std::vector<u8> img(1024 * 512, 0);
    plant_fat_boot(img, 16, 1, 0, 512, 900, 20, 0, false);
    EXPECT_EQ(fat::lost_boot_flavor(img.data() + 16 * 512), 0)
        << "numFATs == 0 is rejected by every deep gate";
    plant_fat_boot(img, 16, 1, 3, 512, 900, 20, 0, false);
    EXPECT_EQ(fat::lost_boot_flavor(img.data() + 16 * 512), 1)
        << "numFATs == 3 passes the deep gates (og: != 0 only)";
    /* ...but the engine-open gates decline it (the og's layering) */
    fat::FatParams p;
    u8 boot[512];
    std::memcpy(boot, img.data() + 16 * 512, 512);
    ASSERT_EQ(fat::validate_boot(boot, 1024 * 512, p), OK);
    EXPECT_NE(fat::boot_passes_open_gates(boot, 512, p), OK)
        << "numFATs == 3 must decline at engine open (og {1,2} gate)";
    plant_fat_boot(img, 16, 1, 2, 512, 900, 20, 0, false);
    EXPECT_EQ(fat::lost_boot_flavor(img.data() + 16 * 512), 1);
}

TEST(LostFatGates, Fat32NameStringTornYieldsNoCandidate) {
    /* N-034: the og's FAT32 deep gate requires the exact-case
     * "FAT32   " memcmp at +0x52; a torn string kills the FAT32
     * candidate. The FAT16 fallback gate (no string) can still claim it
     * when the bracket order allows - here the FAT32-bracket volume
     * with a torn string and spf16 == 0 falls through every gate. */
    std::vector<u8> img(1024 * 512, 0);
    /* 66000 total, spf32 512, no root -> ~65k clusters (FAT32 bracket) */
    plant_fat_boot(img, 16, 1, 2, 0, 66000, 0, 512, false);
    EXPECT_EQ(fat::lost_boot_flavor(img.data() + 16 * 512), 0)
        << "torn FAT32 string + no spf16 must reject";
    plant_fat_boot(img, 16, 1, 2, 0, 66000, 0, 512, true);
    EXPECT_EQ(fat::lost_boot_flavor(img.data() + 16 * 512), 3)
        << "intact FAT32 string yields the FAT32 candidate";
}

TEST(LostFatGates, Fat12SmallFatProbe) {
    /* N-034: the FAT12 gate carries the og's small-FAT probe
     * (FUN_1800048f0 :2630-2632): spf16 must be in [1,200]. A
     * small-bracket boot with spf16 > 200 fails the FAT12 gate but is
     * claimed by the FAT16 fallback gate (spf16 != 0, no cap) - the
     * og ladder's bracket-order semantics. */
    std::vector<u8> img(1024 * 512, 0);
    plant_fat_boot(img, 16, 1, 2, 512, 900, 201, 0, false);
    EXPECT_EQ(fat::lost_boot_flavor(img.data() + 16 * 512), 2)
        << "spf16 = 201: FAT12 gate fails (cap 200), FAT16 fallback claims it";
    plant_fat_boot(img, 16, 1, 2, 512, 900, 150, 0, false);
    EXPECT_EQ(fat::lost_boot_flavor(img.data() + 16 * 512), 1)
        << "spf16 = 150 passes the FAT12 small-FAT probe";
    /* no gate passes: spf16 = 0 (FAT12/FAT16 fail), no FAT32 string,
     * small bracket */
    plant_fat_boot(img, 16, 1, 2, 512, 900, 0, 0, false);
    EXPECT_EQ(fat::lost_boot_flavor(img.data() + 16 * 512), 0)
        << "spf16 = 0 + no string: every deep gate rejects";
}

TEST(LostFatGates, Fat12ProbeSpfBoundIsExclusiveAt200) {
    /* N-034 precision: the og test is `spf == 0 || spf > 200 -> fail`
     * (:2630-2632), so 200 itself passes. */
    std::vector<u8> img(1024 * 512, 0);
    plant_fat_boot(img, 16, 1, 2, 512, 900, 200, 0, false);
    EXPECT_EQ(fat::lost_boot_flavor(img.data() + 16 * 512), 1);
}

TEST(LostFatGates, Fat32GateStringIsCaseSensitive) {
    /* N-034: the og's deep FAT32 gate uses memcmp (case-sensitive),
     * unlike the engine-open _strnicmp - "fat32   " fails the deep
     * gate. */
    std::vector<u8> img(1024 * 512, 0);
    plant_fat_boot(img, 16, 1, 2, 0, 66000, 0, 512, true);
    std::memcpy(img.data() + 16 * 512 + 0x52, "fat32   ", 8);
    EXPECT_EQ(fat::lost_boot_flavor(img.data() + 16 * 512), 0);
}

TEST(LostDiscovery, FatBootWithOgGatesBecomesCandidate) {
    /* Integration: a FAT16 boot that passes the og deep gates surfaces
     * as a Fat candidate through the full discovery pass. */
    auto img = build_disk();
    plant_fat_boot(img, 6000, 1, 2, 512, 900, 20, 0, false);
    MemDevice dev(std::move(img));
    table::DiskTable t;
    ASSERT_EQ(table::parse_disk_table(&dev, t), OK);
    DiscoveryResult r;
    ASSERT_EQ(discover(&dev, t.partitions, 1, nullptr, nullptr, nullptr, r), OK);
    const Candidate* fat_c = nullptr;
    for (const auto& c : r.candidates)
        if (c.start_lba == 6000)
            fat_c = &c;
    ASSERT_NE(fat_c, nullptr) << "valid FAT16 boot must surface as a candidate";
    EXPECT_EQ(fat_c->fs, CandidateFs::Fat);
}

TEST(LostDiscovery, Fat32ExtentUsesDataAreaForContainment) {
    /* N-035: the og's containment math is anchored at the DATA AREA
     * (EndSec = DataAreaStart + totalSec; DRWUI_decompiled.c
     * :842476-842479; CalcPartition.log: FAT32 candidates carry
     * StartSec->DataAreaStart gaps of 472011..2724233 sectors). A peer
     * whose boot sector sits inside the outer's reserved+FATs region is
     * NOT contained by the og - the metadata gap spans it, the data
     * area does not. Pre-fix the whole-volume extent swallowed it. */
    /* outer FAT16 at 16: reserved 2 + 2 FATs x 300 + root 32 =
     * first_data 634 (data area from 650); total 40000, spc 8 ->
     * ~4920 clusters (FAT16 bracket). Inner FAT12 at 316 - inside the
     * outer's SECOND FAT region [18..618), before the data area. */
    std::vector<u8> img(u64(40960) * 512, 0);
    plant_fat_boot(img, 16, 8, 2, 512, 40000, 300, 0, false);
    plant_fat_boot(img, 316, 8, 2, 512, 2000, 20, 0, false);
    MemDevice dev(std::move(img));
    DiscoveryResult r;
    ASSERT_EQ(discover(&dev, {}, 1, nullptr, nullptr, nullptr, r), OK);
    const Candidate* outer = nullptr;
    const Candidate* inner = nullptr;
    int fat_count = 0;
    for (const auto& c : r.candidates) {
        if (c.fs != CandidateFs::Fat)
            continue;
        ++fat_count;
        if (c.start_lba == 16)
            outer = &c;
        if (c.start_lba == 316)
            inner = &c;
    }
    ASSERT_NE(outer, nullptr);
    EXPECT_NE(inner, nullptr)
        << "the peer inside the outer's FAT region is not contained by the data-area extent";
    EXPECT_GE(fat_count, 2);
}

TEST(LostDiscovery, LostCandidateReportsDataAreaOffset) {
    /* N-035: the Candidate carries BOTH extents - the whole-volume one
     * (materialize/slicing need the metadata region) and the og's
     * data-area one (DataAreaStart + totalSec; the QA log's totalSec).
     * FAT is the metadata-gap family; NTFS shows DataAreaStart ==
     * StartSec in the corpus log. */
    std::vector<u8> img(u64(40960) * 512, 0);
    plant_fat_boot(img, 16, 8, 2, 512, 40000, 300, 0, false);
    plant_ntfs(img, 2048, 4000, 100);
    MemDevice dev(std::move(img));
    DiscoveryResult r;
    ASSERT_EQ(discover(&dev, {}, 1, nullptr, nullptr, nullptr, r), OK);
    const Candidate* fat_c = nullptr;
    const Candidate* ntfs_c = nullptr;
    for (const auto& c : r.candidates) {
        if (c.fs == CandidateFs::Fat && c.start_lba == 16)
            fat_c = &c;
        if (c.fs == CandidateFs::Ntfs && c.start_lba == 2048)
            ntfs_c = &c;
    }
    ASSERT_NE(fat_c, nullptr);
    ASSERT_NE(ntfs_c, nullptr);
    /* FAT: reserved 2 + 2x300 FATs + 32 root = first_data 634 */
    EXPECT_EQ(fat_c->sector_count, 40000u); /* whole-volume extent intact */
    EXPECT_EQ(fat_c->data_start_lba, 16u + 634u);
    EXPECT_EQ(fat_c->data_sector_count, 40000u - 634u);
    /* NTFS: DataAreaStart == StartSec (the corpus log's no-gap shape) */
    EXPECT_EQ(ntfs_c->data_start_lba, ntfs_c->start_lba);
    EXPECT_EQ(ntfs_c->data_sector_count, ntfs_c->sector_count);
}

/* ---------------------------------------------------------------- */
/* M-017: the og's content-driven scoring (DRWUI FUN_14050fda0,     */
/* verified to 6 decimals against all 187 CalcPartition.log rows)   */
/* ---------------------------------------------------------------- */

namespace {

/* plant a WALKABLE FAT16 volume at `lba` with `nfiles` real root
 * files (the FAT engine's own scan counts them through the lost
 * content probe) */
void plant_fat16_content(std::vector<u8>& img, u64 lba, u16 nfiles) {
    /* 8 MiB: (16384 - 74)/1 = 16310 data clusters - above the 4085
     * FAT16/FAT12 boundary (a 1 MiB volume would decode as FAT12) */
    const u64 vol_sectors = 16384;
    u8* b = img.data() + lba * 512;
    std::memcpy(b + 3, "MSWIN4.1", 8);
    put16(img, lba * 512 + 0x0B, 512); /* bps */
    b[0x0D] = 1;                       /* spc */
    put16(img, lba * 512 + 0x0E, 2);   /* reserved */
    b[0x10] = 2;                       /* numfats (N-034 gate: 1..2) */
    put16(img, lba * 512 + 0x11, 512); /* root entries */
    put16(img, lba * 512 + 0x13, u16(vol_sectors));
    b[0x15] = 0xF8;                    /* media */
    put16(img, lba * 512 + 0x16, 20);  /* spf */
    put16(img, lba * 512 + 0x18, 63);  /* spt */
    put16(img, lba * 512 + 0x1A, 255); /* heads */
    b[0x26] = 0x29;
    std::memcpy(b + 0x2B, "LOSTFAT    ", 11);
    std::memcpy(b + 0x36, "FAT16   ", 8);
    b[510] = 0x55;
    b[511] = 0xAA;
    /* FAT1 @+2, FAT2 @+22, ROOT @+42 (512 entries x 32B = 32 sectors),
     * DATA @+74 */
    const u64 FAT1 = (lba + 2) * 512, FAT2 = (lba + 22) * 512;
    const u64 ROOT = (lba + 42) * 512, DATA = (lba + 74) * 512;
    put16(img, FAT1 + 0, 0xFFF8); /* M-015 canonical media word */
    put16(img, FAT1 + 2, 0xFFFF);
    put16(img, FAT2 + 0, 0xFFF8);
    put16(img, FAT2 + 2, 0xFFFF);
    for (u16 i = 0; i < nfiles; ++i) {
        const u32 cl = 2 + i;
        put16(img, FAT1 + cl * 2, 0xFFFF); /* one-cluster chain */
        put16(img, FAT2 + cl * 2, 0xFFFF);
        u8* e = img.data() + ROOT + size_t(i) * 32;
        std::memset(e, 0, 32);
        e[0] = 'F';
        e[1] = u8('0' + (i / 10) % 10);
        e[2] = u8('0' + i % 10);
        e[3] = ' ';
        e[4] = 'T';
        e[5] = 'X';
        e[6] = 'T';
        e[11] = 0x20;                                   /* archive */
        put16(img, ROOT + size_t(i) * 32 + 14, 0x5821); /* write date (valid) */
        put16(img, ROOT + size_t(i) * 32 + 16, 0x6000); /* write time */
        put16(img, ROOT + size_t(i) * 32 + 18, 0x5821); /* access date */
        put16(img, ROOT + size_t(i) * 32 + 26, u16(cl));
        put32v(img, ROOT + size_t(i) * 32 + 28, 100);
        std::memcpy(img.data() + DATA + (cl - 2) * 512, "DATA", 4);
    }
}

} // namespace

TEST(LostScoring, MatchesLogRows) {
    /* the formula reproduced from CalcPartition.log rows (the og's own
     * executed output, verified to 6 decimals in the finding). The
     * 429/168849 row is exact; the other rows' scan maxima are not
     * printed in the log, so they are recovered by inversion from the
     * logged 6-decimal ratios (hence the 1e-5 band). */
    EXPECT_NEAR(score_ratio(429, 168849), 0.503579, 1e-6);
    /* (0 files, 25 folders) FAT32 + bonus 20 -> count 1270 */
    EXPECT_NEAR(score_ratio(weighted_count(1, 0, 25) + 20, 199961), 0.585518, 1e-5);
    /* the count==1 identity quirk: 1 file outranks a 2-file candidate
     * (ln(2) < 1 < ln(3) - the quirk's true extent) */
    EXPECT_NEAR(score_ratio(1, 199924), 0.081928, 1e-5);
    EXPECT_GT(score_ratio(1, 199924), score_ratio(2, 199924));
    /* degenerate denominators stay 1.0 */
    EXPECT_DOUBLE_EQ(score_ratio(1, 1), 1.0);
    EXPECT_DOUBLE_EQ(score_ratio(0, 1), 1.0);
    /* the og's per-FS weights (FUN_14050fda0 :842176-842200) */
    EXPECT_EQ(u64(429 + 50 * 3), weighted_count(3, 429, 3)); /* NTFS family */
    EXPECT_EQ(u64(10 + 10 * 3), weighted_count(8, 10, 3));   /* HFS+ f+10d */
    EXPECT_EQ(u64((7 + 3) * 50), weighted_count(1, 7, 3));   /* FAT (f+d)*50 */
    EXPECT_EQ(u64((7 + 3) * 50), weighted_count(2, 7, 3));   /* FAT */
}

TEST(LostDiscovery, FileRichCandidateOutranksCleanBoot) {
    /* M-017's falsifiable claim: a file-rich candidate outranks a
     * file-poor one with CLEANER structural evidence - the og's score
     * measures recovery value, not signature plausibility. The old
     * absolute-points model ranked the clean NTFS (70) above the FAT
     * (30) regardless of content. */
    std::vector<u8> img(u64(32768) * 512, 0); /* 16 MiB disk */
    plant_ntfs(img, 1024, 512, 64);           /* clean NTFS: 70 pts, no content */
    plant_fat16_content(img, 4096, 40);       /* rich FAT16: 30 pts, 40 files */
    MemDevice dev(std::move(img));
    DiscoveryResult r;
    ASSERT_EQ(discover(&dev, {}, 1, nullptr, nullptr, nullptr, r), OK);
    ASSERT_GE(r.candidates.size(), 2u);
    /* the FAT's weighted count (40*50 + 20 = 2020) dominates the empty
     * NTFS's (0 + 60) - ln(60)/ln(2020) = 537 permille */
    EXPECT_EQ(r.candidates[0].fs, CandidateFs::Fat) << "content must outrank structure";
    EXPECT_EQ(r.candidates[0].score, 1000);
    ASSERT_EQ(r.candidates[1].fs, CandidateFs::Ntfs);
    EXPECT_EQ(r.candidates[1].score, 537);
    EXPECT_EQ(r.candidates[1].content_count, u64(60));
}

TEST(LostDiscovery, TableMatchBonus500) {
    /* "Found once partition:%I64d": a candidate whose start matches a
     * live table entry gains +500 on its count (DRWUI :842260-66). The
     * core's old model INVERTED this polarity (overlap penalties). */
    std::vector<u8> img(u64(32768) * 512, 0);
    plant_ntfs(img, 2048, 512, 64);
    plant_fat16_content(img, 8192, 40);
    MemDevice dev(std::move(img));
    table::PartitionInfo live;
    live.start_lba = 2048;
    live.sector_count = 512;
    live.used = true;
    live.fs = table::FsKind::Ntfs;
    std::vector<table::PartitionInfo> known{live};
    DiscoveryResult r;
    ASSERT_EQ(discover(&dev, known, 1, nullptr, nullptr, nullptr, r), OK);
    const Candidate* ntfs_c = nullptr;
    for (const auto& c : r.candidates)
        if (c.fs == CandidateFs::Ntfs)
            ntfs_c = &c;
    ASSERT_NE(ntfs_c, nullptr);
    EXPECT_TRUE(ntfs_c->table_match);
    /* 0 files + NTFS bonus 60 + table-match 500 = 560 */
    EXPECT_EQ(ntfs_c->content_count, u64(560));
    /* ln(560)/ln(2020) = 831 permille (without the bonus it would be
     * 537 - the FAT at 2020 stays ahead, the og's relative model) */
    EXPECT_EQ(ntfs_c->score, 831);
}

TEST(LostDiscovery, OverLargeDuplicateRescoredToFront) {
    /* AdjustPartitionInfoOrder :842776-93 + :842851: the first
     * size-matching candidate AFTER at least one non-matching one (the
     * iVar16 counter) gets the leader's score copied and is spliced to
     * result #2 (log-proven round 3). A matcher adjacent to the leader
     * takes the break path instead. */
    std::vector<u8> img(u64(131072) * 512, 0); /* 64 MiB disk */
    plant_fat16_content(img, 4096, 40);        /* leader: 2020, 16384 sectors */
    plant_ntfs(img, 1024, 65536, 64);          /* NON-matcher: 32 MiB vs 8 MiB */
    plant_ntfs(img, 70000, 4096, 64);          /* matcher: 2 MiB within 16 MiB */
    MemDevice dev(std::move(img));
    DiscoveryResult r;
    ASSERT_EQ(discover(&dev, {}, 1, nullptr, nullptr, nullptr, r), OK);
    ASSERT_GE(r.candidates.size(), 3u);
    EXPECT_EQ(r.candidates[0].fs, CandidateFs::Fat);
    EXPECT_EQ(r.candidates[0].score, 1000);
    /* the size-matched candidate: spliced to #2 with the leader's
     * score (the non-matching NTFS at 1024 precedes it) */
    EXPECT_EQ(r.candidates[1].start_lba, u64(70000));
    EXPECT_EQ(r.candidates[1].score, r.candidates[0].score);
    /* the non-matching candidate keeps its own ln score and #3 */
    EXPECT_EQ(r.candidates[2].start_lba, u64(1024));
    EXPECT_EQ(r.candidates[2].score, 537);
}
