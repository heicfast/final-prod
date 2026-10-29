/* M-143 — ext4 journal inode-image harvest.
 *
 * The og's jbd2 pass classifies every in-range journal block: first the
 * '.'/'..' directory-entry test, then — on failure — an inode-table-image
 * scan that walks the block at inode_size stride, accepting records with
 * i_size != 0, EXTENTS_FL set, extent magic 0xF30A and eh_entries != 0,
 * then emits file/dir records named "iNode%N" (FUN_180005e50 :4414-4573;
 * orphan-finalize consumer FUN_1800041e0 :2982-2993). The core's prior
 * journal pass parsed directories only — journal-recoverable files whose
 * directory blocks were gone were invisible.
 *
 * Test cases:
 *   - JournalInodeImageSurfacesInodeRecord: an inode-image block (no
 *     '.'/'..' pair) with a valid regular-file inode at slot 0 surfaces
 *     as an "Inode%N" record under Lost Files with the inode's extents.
 *     RED pre-fix (the inode-image scan was absent); GREEN post-fix.
 *   - JournalInodeImageZeroSizeSkipped: i_size_lo == 0 → not surfaced.
 *   - JournalInodeImageNoExtentsFlagSkipped: EXTENTS_FL unset → not surfaced.
 *   - JournalInodeImageBadExtentMagicSkipped: extent magic != 0xF30A → not
 *     surfaced.
 *   - JournalInodeImageZeroEhEntriesSkipped: eh_entries == 0 → not surfaced.
 *   - JournalDirPairTakesPrecedence: a block that has BOTH a leading
 *     '.'/..' dirent pair AND a valid inode-image at slot 0 — the
 *     directory model wins per the og's branch order.
 */
#include "cr/ext4/cr_ext4_engine.h"
#include "cr/cr_error.h"
#include "cr/tree/cr_node.h"
#include <gtest/gtest.h>
#include <cstring>
#include <vector>

namespace cr {
namespace ext4 {
namespace {

/* minimal ext4 fixture, modeled on test_ext4_volume.cpp's helpers but
 * standalone so this test file is self-contained. BS=1024 (so fdb=1),
 * one group, 8 inodes, inode_size=128 — the journal sits at blocks
 * 10..14. */
constexpr u32 BS = 1024;
constexpr u32 BPG = 8192;
constexpr u32 IPG = 16;
constexpr u32 INO_SZ = 128;
constexpr u64 B_JOURNAL = 10;
constexpr u64 B_INODE_TABLE = 3;
constexpr u64 B_BITMAP = 4;
constexpr u64 B_ROOTDIR = 5;

void put16(u8* p, u16 v) { p[0] = u8(v); p[1] = u8(v >> 8); }
void put32(u8* p, u32 v) {
    p[0] = u8(v); p[1] = u8(v >> 8); p[2] = u8(v >> 16); p[3] = u8(v >> 24);
}
void bput16(u8* p, u16 v) { p[0] = u8(v >> 8); p[1] = u8(v); }
void bput32(u8* p, u32 v) {
    p[0] = u8(v >> 24); p[1] = u8(v >> 16); p[2] = u8(v >> 8); p[3] = u8(v);
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

void add_dirent(u8* blk, u32& cur, u32 ino, const char* name, u8 type) {
    const u32 nlen = u32(strlen(name));
    const u32 rec = (8 + nlen + 3) & ~3u;
    put32(blk + cur, ino);
    put16(blk + cur + 4, u16(rec));
    blk[cur + 6] = u8(nlen);
    blk[cur + 7] = type;
    memcpy(blk + cur + 8, name, nlen);
    cur += rec;
}

void write_sb(u8* img) {
    u8* sb = img + 1024;
    put32(sb + 0x00, 8);             /* inodes_count */
    put32(sb + 0x04, BPG);           /* blocks_count_lo */
    put32(sb + 0x14, 1);             /* first_data_block (1KiB => 1) */
    put32(sb + 0x18, 0);             /* log_block_size = 0 (1024) */
    put32(sb + 0x20, BPG);           /* blocks_per_group */
    put32(sb + 0x28, IPG);           /* inodes_per_group */
    put16(sb + 0x38, 0xEF53);        /* magic */
    put32(sb + 0x4C, 1);             /* rev level 1 */
    put16(sb + 0x58, INO_SZ);        /* inode_size */
    put32(sb + 0x60, 0x0002);        /* incompat: FILETYPE */
    put32(sb + 0xE0, 8);             /* s_journal_inum = 8 (the journal inode) */
}

void write_inode(u8* img, u32 ino, u16 mode, u64 size, u64 block, u32 nblocks,
                 u16 links, u32 dtime) {
    u8* p = img + B_INODE_TABLE * BS + (ino - 1) * INO_SZ;
    memset(p, 0, INO_SZ);
    put16(p + 0x00, mode);
    put32(p + 0x04, u32(size & 0xFFFFFFFFu));     /* i_size_lo */
    put32(p + 0x08, 1000);                        /* atime */
    put32(p + 0x0C, 2000);                        /* ctime */
    put32(p + 0x10, 3000);                        /* mtime */
    put32(p + 0x14, dtime);
    put16(p + 0x1A, links);
    put32(p + 0x1C, nblocks * (BS / 512));        /* i_blocks_lo (512-byte) */
    put32(p + 0x20, 0x80000);                     /* EXTENTS_FL */
    if ((mode & 0xF000) == 0x8000)
        put32(p + 0x6C, u32(size >> 32));          /* i_size_high (regular) */
    /* extent root inline in i_block: header + 1 extent (matches the
     * existing test_ext4_volume.cpp convention) */
    u8* ib = p + 0x28;
    put16(ib + 0, 0xF30A);                        /* magic */
    put16(ib + 2, 1);                             /* entries */
    put16(ib + 4, 4);                             /* max */
    put16(ib + 6, 0);                             /* depth (leaf) */
    put32(ib + 8, 0);                             /* generation */
    put32(ib + 12, 0);                            /* logical block 0 */
    put16(ib + 16, u16(nblocks));                 /* len in blocks */
    put16(ib + 18, 0);                            /* start_hi */
    put32(ib + 20, u32(block));                   /* start_lo */
}

/* write a 5-block journal at B_JOURNAL..B_JOURNAL+4:
 *   jb=0 jsb (magic, blocktype 3, seq=9, s_maxlen=5, s_first=0, incompat=0)
 *   jb=1 descriptor (blocktype 1, seq=9, one 8-byte tag pointing at jb=2
 *       with no SAME_UUID -> 16-byte journal UUID after the tag; second
 *       tag with LAST_TAG only)
 *   jb=2 the journal data block — caller overwrites this with the test
 *       fixture's content (an inode-image block, a dir-pair block, etc.)
 *   jb=3 unused (referenced by the second tag's blocknr but the data is
 *       zero — kept here so the descriptor's tag count matches the
 *       journal's s_maxlen accounting)
 *   jb=4 commit (blocktype 2, seq=9)
 */
void write_journal_skeleton(u8* img) {
    u8* jsb = img + B_JOURNAL * BS;
    bput32(jsb + 0, 0xC03B3998u);
    bput32(jsb + 4, 3);              /* JBD2_SUPERBLOCK_V1 */
    bput32(jsb + 8, 9);              /* sequence */
    bput32(jsb + 0x10, 5);           /* s_maxlen */
    bput32(jsb + 0x14, 0);           /* s_first */
    bput32(jsb + 0x28, 0);           /* s_feature_incompat: none */

    u8* desc = img + (B_JOURNAL + 1) * BS;
    bput32(desc + 0, 0xC03B3998u);
    bput32(desc + 4, 1);             /* JBD2_DESCRIPTOR_BLOCK */
    bput32(desc + 8, 9);             /* sequence */
    /* tag1: blocknr=40, no LAST, no SAME_UUID -> 16-byte UUID follows */
    bput32(desc + 12, 40);
    bput16(desc + 16, 0);            /* checksum */
    bput16(desc + 18, 0);            /* flags: no LAST, no SAME_UUID */
    for (int i = 0; i < 16; ++i)
        desc[20 + i] = u8(0xA0 + i); /* journal UUID */
    /* tag2: LAST_TAG, no block referenced meaningfully */
    bput32(desc + 36, 41);
    bput16(desc + 40, 0);
    bput16(desc + 42, 1);            /* LAST_TAG */

    u8* commit = img + (B_JOURNAL + 4) * BS;
    bput32(commit + 0, 0xC03B3998u);
    bput32(commit + 4, 2);           /* JBD2_COMMIT_BLOCK */
    bput32(commit + 8, 9);           /* sequence */
}

/* plant an inode-image at jb=2 slot 0 (the engine reads jb=2 as the only
 * data block per the descriptor). The caller customizes mode/size/flags/
 * extent-magic/eh-entries via the gate-overrides. */
void plant_inode_image_block(u8* img, u16 mode, u32 size_lo, u32 flags,
                             u16 ext_magic, u16 eh_entries, u32 extent_block) {
    u8* blk = img + (B_JOURNAL + 2) * BS;
    memset(blk, 0, BS);
    /* slot 0 (offset 0, inode_size=128) */
    u8* p = blk;
    put16(p + 0x00, mode);
    put32(p + 0x04, size_lo);
    put32(p + 0x20, flags);
    /* inline extent root at +0x28 (I_BLOCK), 12-byte header then extents */
    put16(p + 0x28, ext_magic);     /* eh_magic */
    put16(p + 0x2A, eh_entries);    /* eh_entries */
    put16(p + 0x2C, 1);             /* eh_max */
    put16(p + 0x2E, 0);             /* eh_depth (leaf) */
    put32(p + 0x30, 0);             /* eh_generation */
    if (ext_magic == 0xF30A && eh_entries != 0) {
        /* first extent at +0x34 (after the 12-byte header) */
        put32(p + 0x34, 0);          /* ee_block (u32, logical) */
        put16(p + 0x38, 1);          /* ee_len (u16) */
        put16(p + 0x3A, 0);          /* ee_start_hi (u16) */
        put32(p + 0x3C, extent_block); /* ee_start_lo (u32) */
    }
}

/* run scan_ext4 over the fixture and collect the lost-root child names. */
struct ScanOutcome {
    u32 rc = 0;
    Ext4ScanStats st{};
    std::vector<std::string> lost_names;
    std::vector<u64> lost_extents; /* first extent's offset for each child */
};

ScanOutcome run_scan(std::vector<u8>& raw) {
    MemDevice dev(raw);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    ScanOutcome out;
    out.rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, nullptr, rl, st);
    out.st = st;
    for (tree::FileNode* c = rl->first_child(); c; c = c->next_sibling()) {
        if (!c->props())
            continue;
        out.lost_names.push_back(c->name_utf8());
        if (!c->props()->extents.empty())
            out.lost_extents.push_back(c->props()->extents[0].offset);
        else
            out.lost_extents.push_back(0);
    }
    return out;
}

std::vector<u8> build_base_image() {
    std::vector<u8> img(BPG * BS, 0);
    write_sb(img.data());
    /* GDT at block 2 (first_data_block=1 + 1) */
    u8* gdt = img.data() + 2 * BS;
    put32(gdt + 0x00, B_BITMAP);     /* bg_block_bitmap */
    put32(gdt + 0x04, 0);            /* bg_inode_bitmap (none) */
    put32(gdt + 0x08, B_INODE_TABLE); /* bg_inode_table */
    /* block bitmap: blocks 0..14 in use, rest free */
    u8* bm = img.data() + B_BITMAP * BS;
    memset(bm, 0xFF, BS);
    for (u64 b = 15; b < BPG; ++b)
        bm[b / 8] &= u8(~(1u << (b % 8)));
    /* root inode (2) and journal inode (8) */
    write_inode(img.data(), 2, 0x4000, BS, B_ROOTDIR, 1, 2, 0);
    write_inode(img.data(), 8, 0x8000, 5 * BS, B_JOURNAL, 10, 1, 0);
    /* root dir: ".", ".." */
    u8* root = img.data() + B_ROOTDIR * BS;
    memset(root, 0, BS);
    u32 cur = 0;
    add_dirent(root, cur, 2, ".", 2);
    add_dirent(root, cur, 2, "..", 2);
    write_journal_skeleton(img.data());
    return img;
}

} // namespace

/* the happy path: an inode-image block with a regular-file inode at slot
 * 0 (size=100, EXTENTS_FL, valid extent header, eh_entries=1, extent at
 * block 50) surfaces as an "Inode%N" record under Lost Files with the
 * extent's offset (50 * 1024). RED pre-fix: the inode-image scan was
 * absent; the dir parser rejects the block and nothing surfaces. */
TEST(Ext4EngineM143, JournalInodeImageSurfacesInodeRecord) {
    auto raw = build_base_image();
    plant_inode_image_block(raw.data(), 0x8000, 100, 0x80000, 0xF30A, 1, 50);
    auto out = run_scan(raw);
    ASSERT_EQ(OK, out.rc) << err_text(out.rc);
    ASSERT_EQ(1u, out.lost_names.size())
        << "the inode-image must surface exactly one record";
    EXPECT_TRUE(out.lost_names[0].starts_with("Inode"))
        << "naming follows the engine's Inode%u convention (og's iNode%I64d form)";
    EXPECT_EQ(u64(50) * BS, out.lost_extents[0])
        << "the extent's offset (block 50 = 51200 bytes) is carried through";
    EXPECT_GE(out.st.journal_salvaged, 1u);
}

TEST(Ext4EngineM143, JournalInodeImageZeroSizeSkipped) {
    auto raw = build_base_image();
    plant_inode_image_block(raw.data(), 0x8000, 0, 0x80000, 0xF30A, 1, 50);
    auto out = run_scan(raw);
    ASSERT_EQ(OK, out.rc);
    EXPECT_EQ(0u, out.lost_names.size())
        << "i_size_lo == 0 -> the gate rejects the record";
}

TEST(Ext4EngineM143, JournalInodeImageNoExtentsFlagSkipped) {
    auto raw = build_base_image();
    plant_inode_image_block(raw.data(), 0x8000, 100, 0, 0xF30A, 1, 50);
    auto out = run_scan(raw);
    ASSERT_EQ(OK, out.rc);
    EXPECT_EQ(0u, out.lost_names.size())
        << "EXTENTS_FL unset -> the gate rejects the record";
}

TEST(Ext4EngineM143, JournalInodeImageBadExtentMagicSkipped) {
    auto raw = build_base_image();
    plant_inode_image_block(raw.data(), 0x8000, 100, 0x80000, 0x1234, 1, 50);
    auto out = run_scan(raw);
    ASSERT_EQ(OK, out.rc);
    EXPECT_EQ(0u, out.lost_names.size())
        << "extent magic != 0xF30A -> the gate rejects the record";
}

TEST(Ext4EngineM143, JournalInodeImageZeroEhEntriesSkipped) {
    auto raw = build_base_image();
    plant_inode_image_block(raw.data(), 0x8000, 100, 0x80000, 0xF30A, 0, 50);
    auto out = run_scan(raw);
    ASSERT_EQ(OK, out.rc);
    EXPECT_EQ(0u, out.lost_names.size())
        << "eh_entries == 0 -> the gate rejects the record";
}

/* directory-model precedence: a block with a leading '.'/..' dirent pair
 * AND a valid inode-image at slot 0 — the directory model wins per the
 * og's branch order; the inode-image scan does NOT run. The directory's
 * child entry surfaces under Lost Files (the existing S-004 path) when
 * its owning dir inode is unreachable in the live tree (the unresolvable
 * chain falls back to root_lost per the existing orphan-finalize path);
 * no "Inode%N" record appears. */
TEST(Ext4EngineM143, JournalDirPairTakesPrecedence) {
    auto raw = build_base_image();
    /* at jb=2, plant a dir block with "."→999 (a ghost dir inode the live
     * tree does not have — so the chain does not resolve and the entries
     * surface flat under root_lost per the existing orphan-finalize) */
    u8* blk = raw.data() + (B_JOURNAL + 2) * BS;
    memset(blk, 0, BS);
    u32 cur = 0;
    add_dirent(blk, cur, 999, ".", 2);     /* ghost dir inode */
    add_dirent(blk, cur, 999, "..", 2);
    add_dirent(blk, cur, 50, "ghost.txt", 1);
    /* also plant a valid inode-image at slot 4 (offset 512) — the dir
     * parser succeeds so the inode-image scan never runs */
    u8* p = blk + 4 * INO_SZ;
    put16(p + 0x00, 0x8000);
    put32(p + 0x04, 200);
    put32(p + 0x20, 0x80000);
    put16(p + 0x28, 0xF30A);
    put16(p + 0x2A, 1);
    put32(p + 0x38, 60);
    auto out = run_scan(raw);
    ASSERT_EQ(OK, out.rc);
    /* the dir path's "ghost.txt" surfaces under Lost Files (the
     * unresolvable-chain fallback); the inode-image's "Inode%N" does NOT
     * — the directory model took precedence */
    bool has_ghost = false;
    bool has_inode_image = false;
    for (const auto& nm : out.lost_names) {
        if (nm == "ghost.txt")
            has_ghost = true;
        if (nm.starts_with("Inode"))
            has_inode_image = true;
    }
    EXPECT_TRUE(has_ghost)
        << "the directory-model path surfaces its child entry under Lost Files";
    EXPECT_FALSE(has_inode_image)
        << "the inode-image scan did NOT run — directory model wins per the og's branch order";
}

} // namespace ext4
} // namespace cr
