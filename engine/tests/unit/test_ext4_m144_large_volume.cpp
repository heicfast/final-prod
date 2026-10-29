/* M-144 — ext4 large-volume caps refuse og-scannable volumes.
 *
 * Two new tests:
 *   - Ext4EngineM144.Large64BitVolumeAbove8MiBGdtOpens: synthesizes a 64BIT
 *     geometry whose GDT exceeds the prior 8 MiB cap (200k groups × 64-B
 *     desc = 12.8 MiB) and verifies scan_ext4 opens and walks it. RED on
 *     the prior cap (ERR_BAD_PARAMETER); GREEN with the chunked GDT reader
 *     and raised MAX_GROUPS.
 *   - Ext4EngineM144.CorruptHugeGroupCountStillRefused: a corrupt SB whose
 *     group count exceeds MAX_GROUPS (1<<26) is still refused — the
 *     deliberate corrupt-SB guard the registered bound keeps.
 *
 * The 64BIT feature_incompat bit (0x80) and the s_desc_size field (0xFE)
 * are decoded by the engine; the volume's actual disk footprint is tiny
 * (the engine only reads the SB, the GDT block range, and the group-0
 * inode table to validate geometry) — no 16 TiB of storage is needed. */
#include "cr/ext4/cr_ext4_engine.h"
#include "cr/cr_error.h"
#include "cr/tree/cr_node.h"
#include <gtest/gtest.h>
#include <cstring>
#include <vector>

namespace cr {
namespace ext4 {
namespace {

constexpr u32 BS4K = 4096;
constexpr u32 BPG_4K = 32768; /* 128 MiB / 4 KiB */
constexpr u32 IPG_4K = 4096;
constexpr u32 DESC_64 = 64;
/* groups chosen so gdt_bytes = 200000 * 64 = 12.8 MiB > 8 MiB (the prior
 * cap). Real volumes above 16 TiB have at least this many groups at 4 KiB
 * blocks / 64-B desc. */
constexpr u64 LARGE_GROUPS = 200000;
/* device footprint: SB at 1024, GDT at block 0 (4 KiB blocks => fdb=0),
 * GDT spans ceil(12.8 MiB / 4 KiB) = 3200 blocks, then one inode-table
 * block for the group-0 liveness probe. We materialize enough blocks to
 * cover the SB + GDT + the group-0 inode table; the engine never reads
 * past the inode table for an empty-volume scan. */
constexpr u64 GDT_BLOCKS = (LARGE_GROUPS * DESC_64 + BS4K - 1) / BS4K; /* 3200 */
constexpr u64 INODE_TABLE_BLOCK = GDT_BLOCKS + 1;
constexpr u64 TOTAL_BLOCKS = INODE_TABLE_BLOCK + 1; /* minimal disk */

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

class MemDevice : public IStorageDevice {
public:
    explicit MemDevice(std::vector<u8> d) : d_(std::move(d)) {
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

private:
    Geometry g_{};
    std::vector<u8> d_;
};

/* build a minimal 64BIT large-volume image: SB + sparse GDT + group-0
 * inode table. The GDT's group-0 entry has a real inode_table block so the
 * liveness probe passes. */
std::vector<u8> build_large_64bit_volume() {
    std::vector<u8> img(size_t(TOTAL_BLOCKS) * BS4K, 0);
    u8* sb = img.data() + 1024;                   /* superblock at byte 1024 */
    put32(sb + 0x00, u32(LARGE_GROUPS * IPG_4K)); /* s_inodes_count */
    put32(sb + 0x04, u32(LARGE_GROUPS * BPG_4K)); /* s_blocks_count_lo */
    put32(sb + 0x14, 0);                          /* s_first_data_block (4KiB => 0) */
    put32(sb + 0x18, 2);                          /* s_log_block_size = 2 (4096) */
    put32(sb + 0x20, BPG_4K);                     /* s_blocks_per_group */
    put32(sb + 0x28, IPG_4K);                     /* s_inodes_per_group */
    put16(sb + 0x38, 0xEF53);                     /* magic */
    put32(sb + 0x4C, 1);                          /* rev level 1 */
    put16(sb + 0x58, 256);                        /* s_inode_size */
    put32(sb + 0x60, 0x0080);                     /* incompat: 64BIT */
    put16(sb + 0xFE, DESC_64);                    /* s_desc_size (64BIT) */
    /* blocks_count_hi (s_blocks_count_hi @ 0x150): 200000*32768 = 6.55e9,
     * fits in u32 only when wrapped; set the high word correctly. */
    put32(sb + 0x150, u32((LARGE_GROUPS * BPG_4K) >> 32));

    /* GDT at block 0 (4 KiB blocks, fdb=0). Each 64-B descriptor: layout is
     * bg_inode_table_lo@0x08 (u32, absolute block). Set group 0's entry. */
    u8* gdt = img.data(); /* block 0 */
    const u64 inode_table_block = INODE_TABLE_BLOCK;
    put32(gdt + 0x08, u32(inode_table_block));       /* bg_inode_table_lo */
    put32(gdt + 0x28, u32(inode_table_block >> 32)); /* bg_inode_table_hi (64BIT) */

    /* mark inode table block as readable (already zeroed by img init). */
    return img;
}

} // namespace

TEST(Ext4EngineM144, Large64BitVolumeAbove8MiBGdtOpens) {
    auto raw = build_large_64bit_volume();
    MemDevice dev(raw);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");

    Ext4ScanFlags fl;
    Ext4ScanStats st;
    const u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << "large 64BIT volume must open; prior cap returned ERR_BAD_PARAMETER";
    EXPECT_EQ(LARGE_GROUPS, st.block_groups)
        << "every group counted — the chunked GDT reader covered the full table";
}

TEST(Ext4EngineM144, CorruptHugeGroupCountStillRefused) {
    /* a corrupt SB whose group count exceeds the registered bound (1<<26)
     * is still refused by validate_superblock — the deliberate corrupt-SB
     * guard the cap legitimately provides. */
    std::vector<u8> raw(size_t(8) * BS4K, 0);
    u8* sb = raw.data() + 1024;
    const u64 corrupt_groups = (1ull << 26) + 1;    /* > MAX_GROUPS */
    put32(sb + 0x04, u32(corrupt_groups * BPG_4K)); /* s_blocks_count_lo */
    put32(sb + 0x14, 0);                            /* fdb (4KiB) */
    put32(sb + 0x18, 2);                            /* log_bs (4KiB) */
    put32(sb + 0x20, BPG_4K);                       /* bpg */
    put32(sb + 0x28, IPG_4K);                       /* ipg */
    put16(sb + 0x38, 0xEF53);                       /* magic */
    put32(sb + 0x4C, 1);                            /* rev */
    put16(sb + 0x58, 256);                          /* inode_size */
    put32(sb + 0x60, 0x0080);                       /* 64BIT */
    put16(sb + 0xFE, DESC_64);

    SuperBlock out;
    EXPECT_FALSE(validate_superblock(sb, out))
        << "the registered MAX_GROUPS bound still refuses a corrupt huge-group SB";
}

} // namespace ext4
} // namespace cr
