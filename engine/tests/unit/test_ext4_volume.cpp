/* test_ext4_volume.cpp - synthetic ext4 volume end-to-end.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/ext4/cr_ext4_engine.h"
#include "cr/session/cr_records.h"
#include "cr/storage/cr_retry.h"
#include <utility>
#include <gtest/gtest.h>
#include <cstring>
#include <functional>
#include <atomic>
#include <map>
#include <set>

namespace cr {
namespace ext4 {
namespace {

constexpr u32 BS = 1024; /* block size - forces first_data_block = 1 */
constexpr u32 IPG = 16;  /* inodes per group */
constexpr u32 BPG = 8192;
constexpr u32 INO_SZ = 128;

class Img {
public:
    Img(u64 blocks) : blocks_(blocks) {
        data_.assign(size_t(blocks) * BS, 0);
        /* GDT at block first_data_block+1 = 2 */
    }
    u8* block(u64 b) { return data_.data() + b * BS; }
    u64 blocks() const { return blocks_; }
    std::vector<u8>& raw() { return data_; }
    size_t size() const { return data_.size(); }

private:
    u64 blocks_;
    std::vector<u8> data_;
};

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
/* jbd2 on-disk structures are BIG-endian (include/linux/jbd2.h) */
void bput16(u8* p, u16 v) {
    p[0] = u8(v >> 8);
    p[1] = u8(v);
}
void bput32(u8* p, u32 v) {
    p[0] = u8(v >> 24);
    p[1] = u8(v >> 16);
    p[2] = u8(v >> 8);
    p[3] = u8(v);
}

/* inode numbers in the fixture */
constexpr u32 INO_ROOT = 2;
constexpr u32 INO_HELLO = 3;
constexpr u32 INO_DIR = 4;
constexpr u32 INO_SUB = 5;
constexpr u32 INO_GONE = 6;

/* blocks: 0 boot, 1 super, 2 GDT, 3 inode table, 4 block bitmap, 5 root dir,
 * 6 hello.txt data, 7 subdir, 8 sub-file data, 9 deleted data */
constexpr u64 B_INODE_TABLE = 3;
constexpr u64 B_BITMAP = 4;
constexpr u64 B_ROOTDIR = 5;
constexpr u64 B_HELLO_DATA = 6;
constexpr u64 B_SUBDIR = 7;
constexpr u64 B_SUB_DATA = 8;
constexpr u64 B_GONE_DATA = 9;

void write_sb(Img& img, u32 inodes_count) {
    u8* sb = img.block(1); /* superblock at byte 1024 = block 1 */
    memset(sb, 0, BS);
    put32(sb + 0x00, inodes_count);      /* s_inodes_count */
    put32(sb + 0x04, u32(img.blocks())); /* s_blocks_count_lo */
    put32(sb + 0x14, 1);                 /* s_first_data_block (1KiB => 1) */
    put32(sb + 0x18, 0);                 /* s_log_block_size = 0 (1024) */
    put32(sb + 0x20, BPG);               /* s_blocks_per_group */
    put32(sb + 0x28, IPG);               /* s_inodes_per_group */
    put16(sb + 0x38, 0xEF53);            /* magic */
    put32(sb + 0x4C, 1);                 /* rev level 1 */
    put16(sb + 0x58, INO_SZ);            /* s_inode_size (u16@0x58; the
                                          * u32 s_first_ino owns 0x54..57) */
    put32(sb + 0x60, 0x0002);            /* incompat: FILETYPE */
    put32(sb + 0xE0, 8);                 /* journal inum */
    memcpy(sb + 0x78, "testvol", 8);
}

void write_gdt(Img& img) {
    u8* g = img.block(2);                /* GDT at block 2 */
    put32(g + 0x00, u32(B_BITMAP));      /* block bitmap */
    put32(g + 0x04, 0);                  /* inode bitmap (unused by engine) */
    put32(g + 0x08, u32(B_INODE_TABLE)); /* inode table */
}

/* write an inode with a single-extent extent-tree */
void write_inode(Img& img, u32 ino, u16 mode, u64 size, u64 extent_block, u32 extent_blocks,
                 u16 links, u32 dtime) {
    u8* table = img.block(B_INODE_TABLE);
    u8* p = table + size_t((ino - 1) * INO_SZ);
    memset(p, 0, INO_SZ);
    put16(p + 0x00, mode);
    put32(p + 0x04, u32(size));
    put32(p + 0x08, 1000); /* atime */
    put32(p + 0x0C, 2000); /* ctime */
    put32(p + 0x10, 3000); /* mtime */
    put32(p + 0x14, dtime);
    put16(p + 0x1A, links);
    /* N-177: i_blocks_lo (512-byte units) - real volumes always carry
     * the coverage word; the fixture now reflects that. */
    put32(p + 0x1C, u32(u64(extent_blocks) * (BS / 512)));
    put32(p + 0x20, 0x80000); /* EXTENTS_FL */
    /* extent root inline in i_block: header + 1 extent */
    u8* ib = p + 0x28;
    put16(ib + 0, 0xF30A); /* magic */
    put16(ib + 2, 1);      /* entries */
    put16(ib + 4, 4);      /* max */
    put16(ib + 6, 0);      /* depth */
    put32(ib + 8, 0);      /* generation */
    /* extent: logical 0, len, phys */
    put32(ib + 12, 0);
    put16(ib + 16, u16(extent_blocks));
    put16(ib + 18, 0);
    put32(ib + 20, u32(extent_block));
}

/* append a dir entry into a 1024-byte dir block */
void add_dirent(u8* dir, u32& cursor, u32 ino, const char* name, u8 type) {
    const u32 nlen = u32(strlen(name));
    const u32 rec = (8 + nlen + 3) & ~3u;
    put32(dir + cursor, ino);
    put16(dir + cursor + 4, u16(rec));
    dir[cursor + 6] = u8(nlen);
    dir[cursor + 7] = type;
    memcpy(dir + cursor + 8, name, nlen);
    cursor += rec;
}

Img build_volume() {
    Img img(BPG); /* one group */
    write_sb(img, 8);
    write_gdt(img);

    /* block bitmap: blocks 0..14 in use, rest free */
    u8* bm = img.block(B_BITMAP);
    memset(bm, 0xFF, BS);
    /* clear bits for blocks 15.. to mark free (deleted data on free blocks) */
    for (u64 b = 15; b < BPG; ++b)
        bm[b / 8] &= u8(~(1u << (b % 8)));

    /* inodes */
    write_inode(img, INO_ROOT, 0x4000, BS, B_ROOTDIR, 1, 2, 0);
    write_inode(img, INO_HELLO, 0x8000, 14, B_HELLO_DATA, 1, 1, 0);
    write_inode(img, INO_DIR, 0x4000, BS, B_SUBDIR, 1, 2, 0);
    write_inode(img, INO_SUB, 0x8000, 22, B_SUB_DATA, 1, 1, 0);
    write_inode(img, INO_GONE, 0x8000, 512, B_GONE_DATA, 1, 0, 5000); /* deleted */

    /* root dir: ".", "..", hello.txt, sub */
    u8* root = img.block(B_ROOTDIR);
    memset(root, 0, BS);
    u32 cur = 0;
    add_dirent(root, cur, INO_ROOT, ".", 2);
    add_dirent(root, cur, INO_ROOT, "..", 2);
    add_dirent(root, cur, INO_HELLO, "hello.txt", 1);
    add_dirent(root, cur, INO_DIR, "sub", 2);
    /* a deleted entry: inode 0 (freed slot) */
    add_dirent(root, cur, 0, "gone-entry", 1);

    /* subdir: ".", "..", nested.bin */
    u8* sub = img.block(B_SUBDIR);
    memset(sub, 0, BS);
    cur = 0;
    add_dirent(sub, cur, INO_DIR, ".", 2);
    add_dirent(sub, cur, INO_ROOT, "..", 2);
    add_dirent(sub, cur, INO_SUB, "nested.bin", 1);

    memcpy(img.block(B_HELLO_DATA), "Hello, world!", 14);
    memcpy(img.block(B_SUB_DATA), "0123456789ABCDEFGHIJK", 22);
    memset(img.block(B_GONE_DATA), 0x77, 512);

    /* ---- jbd2 journal (blocks 10..14), BIG-endian (jbd2.h) ---- */
    constexpr u64 B_JOURNAL = 10;
    write_inode(img, 8 /* journal inum per SB 0xE0 */, 0x8000, 5 * BS, B_JOURNAL, 5, 1, 0);
    {
        /* jsb at journal block 0: {magic BE, blocktype 3 BE, seq BE}
         * s_feature_incompat BE32@0x28 = 0 -> plain 8-byte tags, flags@+6 */
        u8* jsb = img.block(B_JOURNAL);
        bput32(jsb + 0, 0xC03B3998u);
        bput32(jsb + 4, 3); /* JBD2_SUPERBLOCK_V1 */
        bput32(jsb + 8, 9);
        bput32(jsb + 0x10, 5); /* S-004: s_maxlen - the declared log range */
        bput32(jsb + 0x14, 0); /* s_first */
        bput32(jsb + 0x28, 0); /* s_feature_incompat: none */

        /* descriptor at journal block 1: tags reference journal-sequential
         * data blocks 2 and 3; first tag carries no SAME_UUID -> 16-byte
         * journal UUID after the tag bytes (commit.c write order) */
        u8* desc = img.block(B_JOURNAL + 1);
        bput32(desc + 0, 0xC03B3998u);
        bput32(desc + 4, 1); /* JBD2_DESCRIPTOR_BLOCK */
        bput32(desc + 8, 9); /* sequence */
        /* tag1 (8 bytes plain): blocknr BE32, checksum BE16, flags BE16@+6 */
        bput32(desc + 12, 40); /* fs block of the snapshot */
        bput16(desc + 16, 0);  /* checksum */
        bput16(desc + 18, 0);  /* flags: no LAST, no SAME_UUID */
        for (int i = 0; i < 16; ++i)
            desc[20 + i] = u8(0xA0 + i); /* journal UUID after tag1 */
        /* tag2: escaped block, LAST_TAG */
        bput32(desc + 36, 41);
        bput16(desc + 40, 0);
        bput16(desc + 42, 1 | 8); /* ESCAPE | LAST_TAG */

        /* journal data block 2: directory snapshot with a deleted name */
        u8* snap = img.block(B_JOURNAL + 2);
        u32 jcur = 0;
        add_dirent(snap, jcur, 50, "ghost.txt", 1);

        /* journal data block 3: ESCAPED directory snapshot - its first four
         * bytes (the first dirent's inode field) collide with the journal
         * magic (inode 0x98393BC0 -> bytes C0 3B 39 98) so the journal copy
         * has them zeroed; the ESCAPE tag restores cpu_to_be32(magic) */
        u8* esc = img.block(B_JOURNAL + 3);
        {
            const char* nm = "esc.txt";
            const u32 nlen = 7;
            const u32 rec = (8 + nlen + 3) & ~3u;
            put32(esc + 0, 0x98393BC0u); /* then zeroed below */
            put16(esc + 4, u16(rec));
            esc[6] = u8(nlen);
            esc[7] = 1;
            memcpy(esc + 8, nm, nlen);
            memset(esc + 0, 0, 4); /* the journal copy is zero-escaped */
        }

        /* commit at journal block 4 (same sequence 9) */
        u8* commit = img.block(B_JOURNAL + 4);
        bput32(commit + 0, 0xC03B3998u);
        bput32(commit + 4, 2); /* JBD2_COMMIT_BLOCK */
        bput32(commit + 8, 9);
    }
    return img;
}

TEST(Ext4Engine, ExtentTreeDepth3) {
    /* N-019 scenario 6 (the depth path no prior test pinned): a file whose
     * extent tree is ROOT(inode, depth 2) -> INDEX -> INDEX -> LEAF -> data.
     * The engine's walk_extent_node recurses depth-1 per level with a node
     * budget; the deepest shape real ext4 allows on large volumes is depth
     * 5. This pins the full resolution chain: every leaf extent lands at
     * the right physical block, non-adjacent runs stay separate extents,
     * adjacent ones coalesce, and the file's logical size survives. */
    auto img = build_volume();
    constexpr u32 INO_DEEP = 7;
    constexpr u64 B_IDX1 = 20, B_IDX2 = 21, B_LEAF = 22;
    constexpr u64 B_DATA_A0 = 23, B_DATA_A1 = 24; /* run A: 23,24 */
    constexpr u64 B_DATA_B0 = 30, B_DATA_B1 = 31; /* run B: 30,31 (non-adjacent) */

    /* inode: depth-2 root header + one index entry -> B_IDX1 */
    {
        u8* p = img.block(B_INODE_TABLE) + size_t((INO_DEEP - 1) * INO_SZ);
        memset(p, 0, INO_SZ);
        put16(p + 0x00, 0x8000); /* regular file */
        put32(p + 0x04, 4 * BS); /* size: 4 blocks */
        put32(p + 0x0C, 2000);
        put32(p + 0x10, 3000);
        put16(p + 0x1A, 1);       /* links */
        put32(p + 0x20, 0x80000); /* EXTENTS_FL */
        u8* ib = p + 0x28;
        put16(ib + 0, 0xF30A);       /* magic */
        put16(ib + 2, 1);            /* entries */
        put16(ib + 4, 4);            /* max */
        put16(ib + 6, 3);            /* depth 3: root -> index -> index -> leaf
                                      * (kernel semantics: depth = levels below
                                      * root; the engine decrements per level) */
        put32(ib + 8, 0);            /* generation */
        put32(ib + 12, 0);           /* ei_block: logical 0 */
        put32(ib + 16, u32(B_IDX1)); /* ei_leaf_lo */
        put16(ib + 20, 0);           /* ei_leaf_hi */
    }
    /* index level 1 (block 20): own header depth 2, one entry -> block 21 */
    {
        u8* n = img.block(B_IDX1);
        memset(n, 0, BS);
        put16(n + 0, 0xF30A);
        put16(n + 2, 1);
        put16(n + 4, 340);          /* max = (1024-12)/12 */
        put16(n + 6, 2);            /* own depth: two levels below this node */
        put32(n + 12, 0);           /* ei_block */
        put32(n + 16, u32(B_IDX2)); /* ei_leaf_lo */
    }
    /* index level 2 (block 21): own header depth 1, one entry -> leaf 22 */
    {
        u8* n = img.block(B_IDX2);
        memset(n, 0, BS);
        put16(n + 0, 0xF30A);
        put16(n + 2, 1);
        put16(n + 4, 340);
        put16(n + 6, 1);
        put32(n + 12, 0);
        put32(n + 16, u32(B_LEAF));
    }
    /* leaf (block 22): depth 0, four extents - 2 adjacent + 2 adjacent,
     * the two runs physically disjoint */
    {
        u8* n = img.block(B_LEAF);
        memset(n, 0, BS);
        put16(n + 0, 0xF30A);
        put16(n + 2, 4);
        put16(n + 4, 340);
        put16(n + 6, 0); /* depth 0 = leaf */
        const u64 runs[4] = {B_DATA_A0, B_DATA_A1, B_DATA_B0, B_DATA_B1};
        for (u32 i = 0; i < 4; ++i) {
            u8* e = n + 12 + i * 12;
            put32(e + 0, i);            /* ee_block: logical */
            put16(e + 4, 1);            /* ee_len */
            put16(e + 6, 0);            /* ee_start_hi */
            put32(e + 8, u32(runs[i])); /* ee_start_lo */
        }
    }
    /* data with per-block markers */
    memset(img.block(B_DATA_A0), 0xA1, BS);
    memset(img.block(B_DATA_A1), 0xA2, BS);
    memset(img.block(B_DATA_B0), 0xB1, BS);
    memset(img.block(B_DATA_B1), 0xB2, BS);

    /* root dirent: append after the existing entries */
    {
        u32 cur = 0;
        const u8* root = img.block(B_ROOTDIR);
        while (cur + 8 <= BS) {
            const u32 rec = u32(root[cur + 4]) | (u32(root[cur + 5]) << 8);
            if (rec == 0)
                break;
            cur += rec;
        }
        ASSERT_LT(cur + 20u, BS) << "root dir must have room for the new entry";
        add_dirent(img.block(B_ROOTDIR), cur, INO_DEEP, "deep.bin", 1);
    }

    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);

    tree::FileNode* deep = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "deep.bin")
                deep = c;
            find(c);
        }
    };
    find(ra);
    ASSERT_NE(deep, nullptr) << "the depth-3 file must surface under Found";
    ASSERT_NE(deep->props(), nullptr);
    /* the whole chain resolved: size + the exact two-run extent shape */
    EXPECT_EQ(u64(4 * BS), deep->props()->logical_size);
    ASSERT_EQ(size_t(2), deep->props()->extents.size())
        << "adjacent leaf extents coalesce; disjoint runs stay separate";
    EXPECT_EQ(u64(B_DATA_A0 * BS), deep->props()->extents[0].offset);
    EXPECT_EQ(u64(2 * BS), deep->props()->extents[0].length);
    EXPECT_EQ(u64(B_DATA_B0 * BS), deep->props()->extents[1].offset);
    EXPECT_EQ(u64(2 * BS), deep->props()->extents[1].length);
    /* content truth: each resolved extent points at its own run's markers */
    u8 buf[2] = {0, 0};
    ASSERT_EQ(OK, dev.read_at(deep->props()->extents[0].offset, buf, 1));
    EXPECT_EQ(0xA1, buf[0]);
    ASSERT_EQ(OK, dev.read_at(deep->props()->extents[0].offset + BS, buf, 1));
    EXPECT_EQ(0xA2, buf[0]);
    ASSERT_EQ(OK, dev.read_at(deep->props()->extents[1].offset, buf, 1));
    EXPECT_EQ(0xB1, buf[0]);
    ASSERT_EQ(OK, dev.read_at(deep->props()->extents[1].offset + BS, buf, 1));
    EXPECT_EQ(0xB2, buf[0]);
}

TEST(Ext4Engine, SuperblockValidation) {
    Img img(BPG);
    write_sb(img, 8);
    SuperBlock sb;
    u8 raw[1024];
    memcpy(raw, img.block(1), 1024);
    EXPECT_TRUE(validate_superblock(raw, sb));
    EXPECT_EQ(1024u, sb.block_size);
    EXPECT_EQ(1u, sb.first_data_block);
    EXPECT_EQ(128u, sb.inode_size);
    EXPECT_EQ(32u, sb.desc_size);
    EXPECT_EQ(8u, sb.journal_inum);

    /* magic broken */
    img.block(1)[0x38] = 0x00;
    memcpy(raw, img.block(1), 1024);
    EXPECT_FALSE(validate_superblock(raw, sb));

    /* first_data_block rule: 1KiB (log=0) with fdb=0 must reject */
    Img img2(BPG);
    write_sb(img2, 8);
    put32(img2.block(1) + 0x14, 0);
    memcpy(raw, img2.block(1), 1024);
    EXPECT_FALSE(validate_superblock(raw, sb));

    /* 4KiB blocks (log=2) with fdb=1 must reject */
    Img img3(BPG);
    write_sb(img3, 8);
    put32(img3.block(1) + 0x14, 1);
    put32(img3.block(1) + 0x18, 2);
    memcpy(raw, img3.block(1), 1024);
    EXPECT_FALSE(validate_superblock(raw, sb));
}

TEST(Ext4Engine, SbBandsPinned) {
    /* N-202 (PARITY-DELTAS D-145): four superblock acceptance-band
     * differences on garbage SBs, each a registered deliberate choice
     * (near-zero real-volume impact - consolidated so none stays
     * accidental):
     *   (1) inode_size == 0 with rev >= 1: the og REJECTS (:3237's
     *       (rev==0) || inode_size != 0 gate); the core falls back to
     *       128 and accepts.
     *   (2) log_bs 7..20: the og ACCEPTS (its 0x400 << log_bs int-sign
     *       form only fails past 20 - 1 GiB blocks are an og defect
     *       class, the I-045 family); the core rejects above 6.
     *   (3) journal_inum: the og copies the field only when
     *       feature_compat HAS_JOURNAL bit 2 is set (:3365-3367); the
     *       core reads it unconditionally and defaults 0 -> 8.
     *   (4) desc_size: the og clamps to [32,64] unconditionally
     *       (:3397-3405); the core picks 64-iff-64BIT (32 otherwise) -
     *       differs only on invalid combos.
     * This pin asserts the four edges as DELIBERATE; if a band is ever
     * changed, this test forces the register to be re-truthed first. */
    u8 raw[1024];
    SuperBlock sb;

    /* (1) inode_size = 0 + rev 1: accepted with the 128 fallback */
    {
        Img img(BPG);
        write_sb(img, 8);
        put16(img.block(1) + 0x58, 0); /* s_inode_size = 0 */
        memcpy(raw, img.block(1), 1024);
        EXPECT_TRUE(validate_superblock(raw, sb))
            << "core accepts the og-rejected inode_size=0 (128 fallback)";
        EXPECT_EQ(128u, sb.inode_size);
    }
    /* (2) log_bs = 7 (128 KiB blocks): refused - the og's int-sign form
     * would accept up to 20 (1 GiB), an og defect class */
    {
        Img img(BPG);
        write_sb(img, 8);
        put32(img.block(1) + 0x14, 0); /* fdb = 0: isolate the log_bs cap
                                        * (fdb=1 would trip the kernel
                                        * fdb x log_bs rule first) */
        put32(img.block(1) + 0x18, 7); /* s_log_block_size = 7 */
        memcpy(raw, img.block(1), 1024);
        EXPECT_FALSE(validate_superblock(raw, sb))
            << "log_bs > 6 refuses (the og accepts 7..20 - nonsense blocks)";
    }
    /* (3) journal_inum = 0 on a non-journal volume: read unconditionally,
     * defaulted to 8 (the og leaves the field untouched without
     * HAS_JOURNAL) */
    {
        Img img(BPG);
        write_sb(img, 8);
        put32(img.block(1) + 0xE0, 0); /* s_journal_inum = 0, no HAS_JOURNAL */
        memcpy(raw, img.block(1), 1024);
        EXPECT_TRUE(validate_superblock(raw, sb));
        EXPECT_EQ(8u, sb.journal_inum) << "the unconditional-read stance: 0 -> the default inode 8";
    }
    /* (4a) 64BIT + desc_size = 32: the core forces 64 (the og's clamp
     * would keep 32); (4b) non-64BIT + desc_size = 64: the core keeps 32 */
    {
        Img img(BPG);
        write_sb(img, 8);
        put32(img.block(1) + 0x60, 0x0002 | 0x80); /* FILETYPE | 64BIT */
        put16(img.block(1) + 0xFE, 32);            /* s_desc_size = 32 */
        memcpy(raw, img.block(1), 1024);
        EXPECT_TRUE(validate_superblock(raw, sb));
        EXPECT_EQ(64u, sb.desc_size) << "64BIT forces the 64-byte descriptor";
    }
    {
        Img img(BPG);
        write_sb(img, 8);
        put16(img.block(1) + 0xFE, 64); /* s_desc_size = 64, NO 64BIT flag */
        memcpy(raw, img.block(1), 1024);
        EXPECT_TRUE(validate_superblock(raw, sb));
        EXPECT_EQ(32u, sb.desc_size) << "non-64BIT keeps the 32-byte descriptor";
    }
}

TEST(Ext4Engine, EndToEndWalk) {
    auto img = build_volume();
    MemDevice dev(img.raw());

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");

    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(1u, st.block_groups);
    EXPECT_EQ(2u, st.files);
    EXPECT_EQ(1u, st.deleted_inodes);

    /* collect names */
    std::set<std::string> found, deleted;
    std::function<void(tree::FileNode*, std::set<std::string>&, const std::string&)> collect =
        [&](tree::FileNode* n, std::set<std::string>& into, const std::string& p) {
            for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
                if (!c->props())
                    continue;
                const std::string nm = c->name_utf8();
                const std::string full = p.empty() ? nm : (p == "/" ? "/" + nm : p + "/" + nm);
                into.insert(full);
                collect(c, into, full);
            }
        };
    collect(ra, found, "");
    collect(rd, deleted, "");

    /* paths carry the synthetic root "/" component */
    EXPECT_TRUE(found.count("/hello.txt"));
    EXPECT_TRUE(found.count("/sub"));
    EXPECT_TRUE(found.count("/sub/nested.bin"));
    EXPECT_TRUE(found.count("/")); /* synthetic root under Found */

    /* the deleted inode (dtime set, links 0) emitted with its extent */
    ASSERT_EQ(1u, deleted.size());
    EXPECT_TRUE(deleted.begin()->starts_with("Inode6"));
    /* extents intact + blocks marked free -> graded viable */
    tree::FileNode* del = rd->first_child();
    ASSERT_NE(nullptr, del);
    EXPECT_EQ(tree::DeletedState::Deleted, del->deleted_state());
    EXPECT_FALSE(del->props()->extents.empty());
    EXPECT_EQ(u64(9 * BS), del->props()->extents[0].offset);
    EXPECT_EQ(u64(BS), del->props()->extents[0].length); /* block-granular: 512B
        file on a 1KiB block reports the full block extent; logical_size
        stays the real 512 */
}

TEST(Ext4Engine, BadMagicRejected) {
    Img img(BPG);
    write_sb(img, 8);
    img.block(1)[0x39] = 0xEE; /* corrupt magic high byte */
    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    EXPECT_EQ(ERR_BAD_MAGIC,
              scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));
}

TEST(Ext4Engine, UnixTimeConversion) {
    EXPECT_EQ(u64(11644473600ULL * 10000000ULL), unix_to_filetime(0));
    EXPECT_EQ(u64((11644473600ULL + 1000) * 10000000ULL), unix_to_filetime(1000));
    /* post-2038: the full-i64 path is exact where the og's _gmtime32_s
     * import (EXT4SearchDLL.dll, api-ms-win-crt-time-l1-1-0) goes garbage
     * past the 32-bit time_t cliff - the I-040 anti-down-fix pin. */
    EXPECT_EQ(u64((11644473600ULL + 2147483648ULL) * 10000000ULL),
              unix_to_filetime(2147483648LL)); /* 2038-01-19T03:14:08Z+1s */
    EXPECT_EQ(u64((11644473600ULL + 4102444800ULL) * 10000000ULL),
              unix_to_filetime(4102444800LL)); /* 2100-01-01T00:00:00Z */
    EXPECT_EQ(u64((11644473600ULL + 2147483648ULL) * 10000000ULL + 1234567ULL / 100ULL),
              unix_to_filetime_ns(2147483648LL, 1234567u));
}

/* I-040 anti-down-fix pin: a directory inode with a nonzero legacy-ACL
 * pointer at +0x6c (the I_SIZE_HIGH slot, meaningful only for regular
 * files) must keep its true 32-bit size. The og's EXT4SearchDLL
 * (FUN_180007510, :5455-5467) CONCATs +0x6c with +0x04 unconditionally
 * for EVERY inode, so legacy-ACL dirs surface polluted garbage sizes;
 * the core's clean conditional (cr_ext4_engine.cpp: high half only when
 * ifmt(mode)==0x8000) is the registered deliberate non-adoption
 * (PARITY-DELTAS D-123). This pin refuses a future "fix" down to the
 * defect. */
/* ---- N-177: the size-exceeds-blocks damaged flag ---------------------- */

TEST(Ext4Engine, SizeExceedsBlocksDamagedDirectoryStaysUnflagged) {
    /* N-177 CONVERGENCE REPAIR (the S53 lane's byte-equivalent landing
     * kept under a distinct name per the superset rule - the cb2abd42
     * convergence merge left BOTH lanes' tests under the same TEST name,
     * a build-breaking redefinition): this one carries the pin the
     * winning 7be438c9 battery lacks - a DIRECTORY whose size exceeds
     * its block coverage stays unflagged (the og's param_6==0
     * files-only gate, EXT4SearchDLL :6300-6303), and the clean-file
     * control the other side. The clean file carries no bit; the
     * truncated regular file carries it; the damaged dir never does. */
    auto img = build_volume();
    /* INO_HELLO: size 14, one 1 KiB extent = 2 sectors of coverage */
    u8* hello = img.block(B_INODE_TABLE) + size_t((INO_HELLO - 1) * INO_SZ);
    put32(hello + 0x1C, 2); /* i_blocks_lo: 2 x 512B covers the 14-byte file */
    /* INO_SUB: size 22 -> make it the truncated one: 100 KiB declared,
     * the same single-block extent (i_blocks stays 2 sectors) */
    u8* sub = img.block(B_INODE_TABLE) + size_t((INO_SUB - 1) * INO_SZ);
    put32(sub + 0x04, 100 * 1024); /* i_size_lo = 100 KiB */
    put32(sub + 0x1C, 2);          /* i_blocks_lo: 1 KiB of coverage */
    /* INO_DIR: a DIRECTORY whose size exceeds its blocks - stays clean
     * (the og's param_6 == 0 files-only gate) */
    u8* diri = img.block(B_INODE_TABLE) + size_t((INO_DIR - 1) * INO_SZ);
    put32(diri + 0x04, 512 * 1024); /* 512 KiB declared over a 1 KiB extent */
    put32(diri + 0x1C, 2);

    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    ASSERT_EQ(OK, scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st))
        << err_text(0);

    tree::FileNode *hello_n = nullptr, *sub_n = nullptr, *dir_n = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props()) {
                if (c->props()->name == std::u16string(u"hello.txt"))
                    hello_n = c;
                if (c->props()->name == std::u16string(u"nested.bin"))
                    sub_n = c;
                if (c->props()->name == std::u16string(u"sub"))
                    dir_n = c;
            }
            find(c);
        }
    };
    find(ra);
    ASSERT_NE(nullptr, hello_n);
    ASSERT_NE(nullptr, sub_n);
    ASSERT_NE(nullptr, dir_n);
    EXPECT_EQ(0, int(hello_n->props()->flags & tree::NODE_FLAG_SIZE_EXCEEDS_BLOCKS))
        << "the covered file carries no damaged bit";
    EXPECT_NE(0, int(sub_n->props()->flags & tree::NODE_FLAG_SIZE_EXCEEDS_BLOCKS))
        << "the 100-KiB-declared/1-KiB-covered file carries the 0x200 bit";
    EXPECT_EQ(0, int(dir_n->props()->flags & tree::NODE_FLAG_SIZE_EXCEEDS_BLOCKS))
        << "directories never take the files-only damage bit";
}

TEST(Ext4Engine, LegacyAclDirSizeClean) {
    auto img = build_volume();
    /* the legacy dir-ACL pointer lives in i_size_high's slot for
     * directories: a plausible allocator/ACL xattr block number */
    u8* dir_inode = img.block(B_INODE_TABLE) + size_t((INO_DIR - 1) * INO_SZ);
    ASSERT_EQ(0x4000, u16(dir_inode[0]) | u16(dir_inode[1] << 8)); /* dir */
    put32(dir_inode + 0x6C, 0x00003039u);                          /* ACL pointer pollution */

    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    ASSERT_EQ(OK, scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st))
        << err_text(0);

    tree::FileNode* sub = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props() && c->props()->name == std::u16string(u"sub"))
                sub = c;
            find(c);
        }
    };
    find(ra);
    ASSERT_NE(nullptr, sub) << "the ACL-carrying dir must still surface";
    /* the true 32-bit size (BS), never the polluted
     * (0x3039 << 32 | BS) the og's unconditional poll would produce */
    EXPECT_EQ(u64(BS), sub->props()->logical_size);
}

TEST(Ext4Engine, Post2038FileTimeConversionStays64Bit) {
    /* I-040 / D-123: the og's _gmtime32_s paths (EXT4SearchDLL :5495/:5506/
     * :6262/:6273) go garbage past 2038 (32-bit time_t); the core's u64
     * FILETIME path must stay exact through and beyond the boundary. */
    const u64 y2038 = 2147483648ULL; /* 2038-01-19T03:14:08Z, the +1 step */
    EXPECT_EQ((11644473600ULL + y2038) * 10000000ULL, unix_to_filetime(y2038));
    const u64 y2100 = 4102444800ULL; /* 2100-01-01T00:00:00Z */
    EXPECT_EQ((11644473600ULL + y2100) * 10000000ULL, unix_to_filetime(y2100));
}

TEST(Ext4Engine, SizeHighFoldIsRegularOnlyNotTheOgUnconditionalDefect) {
    /* I-040 / D-123 anti-down-fix pin. The og folds the +0x6C word into
     * EVERY inode's size (EXT4SearchDLL FUN_180007510 :5455-5467 - the
     * unconditional high-half poll), so a legacy-ACL directory's ACL
     * pointer pollutes its surfaced size; the core folds only for regular
     * files (cr_ext4_engine.cpp read_inode, the ifmt==0x8000 gate). Both
     * directions pinned so no future session "fixes" the core toward
     * either defect. */
    auto img = build_volume();
    u8* table = img.block(B_INODE_TABLE);
    /* (a) the subdir inode carries a legacy-ACL pointer at +0x6C: the og
     * computes 0x00000300_00000400 for this shape; the core surfaces 1024. */
    put32(table + size_t((INO_DIR - 1) * INO_SZ) + 0x6C, 0x00000300u);
    /* (b) the regular file's true size is hi=1, lo=0xFFFFFFFF (8 GiB-1):
     * the regular-file path must surface the full 64-bit size. */
    put32(table + size_t((INO_HELLO - 1) * INO_SZ) + 0x04, 0xFFFFFFFFu);
    put32(table + size_t((INO_HELLO - 1) * INO_SZ) + 0x6C, 1u);

    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st);
    ASSERT_EQ(OK, rc) << err_text(rc);

    const tree::FileNode* sub = nullptr;
    const tree::FileNode* hello = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (!c->props())
                continue;
            const std::string nm = c->name_utf8();
            if (nm == "sub")
                sub = c;
            if (nm == "hello.txt")
                hello = c;
            find(c);
        }
    };
    find(ra);
    ASSERT_NE(nullptr, sub);
    ASSERT_NE(nullptr, hello);
    EXPECT_EQ(u64(1024), sub->props()->logical_size); /* NOT 0x300_00000400 */
    EXPECT_EQ((u64(1) << 32) | 0xFFFFFFFFu, hello->props()->logical_size);
}

TEST(Ext4Engine, UnwrittenExtentLenDecodedKernelCorrect) {
    /* I-045 / D-141 anti-down-fix pin (the og-defect non-adoption register,
     * batch #3). The og multiplies the RAW ee_len word by the block size
     * with no unwritten-bit handling anywhere in its extent decoder
     * (EXT4SearchDLL FUN_180005670 :4044-4048 - the
     * `CONCAT24(psVar5[-1],...) * (longlong)*(int *)(param_1 + 0x58)` and
     * the `(ushort)psVar5[-2] * block_size` raw products), so an unwritten
     * extent (ee_len bit 15 set - fallocate output, common on modern ext4)
     * yields an overflow-class garbage run length. The core decodes the
     * kernel form (cr_ext4_engine.cpp walk_extent_node :346-350: raw
     * ee_len > 0x8000 -> raw & 0x7FFF) - an unwritten 5-block extent
     * surfaces a 5-block content extent. Future sessions must not "fix"
     * the core down to the og's garbage math. */
    auto img = build_volume();
    /* hello.txt's inline extent root: ee_len := 0x8005 (unwritten, 5 blocks) */
    u8* hello = img.block(B_INODE_TABLE) + size_t((INO_HELLO - 1) * INO_SZ);
    /* fixture sanity: EXTENTS_FL (0x80000) set, extent magic 0xF30A at i_block */
    ASSERT_EQ(0x80000u, u32(hello[0x20]) | u32(hello[0x21] << 8) | u32(hello[0x22] << 16) |
                            u32(u32(hello[0x23]) << 24));
    ASSERT_EQ(0xF30Au, u16(hello[0x28]) | u16(u16(hello[0x29]) << 8));
    put16(hello + 0x28 + 16, 0x8005); /* ee_len @ extent+4 */

    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st);
    ASSERT_EQ(OK, rc) << err_text(rc);

    const tree::FileNode* hello_node = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props() && c->props()->name == std::u16string(u"hello.txt"))
                hello_node = c;
            find(c);
        }
    };
    find(ra);
    ASSERT_NE(nullptr, hello_node) << "the unwritten-extent file must still surface";
    ASSERT_EQ(size_t(1), hello_node->props()->extents.size());
    /* kernel decode: 0x8005 -> 5 blocks (5120 bytes) at the extent's phys */
    EXPECT_EQ(u64(B_HELLO_DATA * BS), hello_node->props()->extents[0].offset);
    EXPECT_EQ(u64(5 * BS), hello_node->props()->extents[0].length);
    /* NOT the og's raw product: 0x8005 * 1024 = 33,556,736 bytes of extent */
    EXPECT_NE(u64(0x8005) * BS, hello_node->props()->extents[0].length);

    /* the S58 convergence lane's unique supplement: the DELETED path decodes
     * the same kernel form - the dtime-marked Inode6's extent re-stamped
     * ee_len 0x8001 (unwritten, 1 block) surfaces a 1-block content extent,
     * never the og's 0x8001 * 1024 raw product. */
    u8* gone6 = img.block(B_INODE_TABLE) + size_t((6 - 1) * INO_SZ);
    put16(gone6 + 0x28 + 16, 0x8001); /* ee_len @ extent+4 */
    MemDevice dev2(img.raw());
    tree::ResultTree tree2;
    auto* ra2 = tree2.add_root(u"Found");
    auto* rd2 = tree2.add_root(u"Deleted");
    Ext4ScanFlags fl2;
    Ext4ScanStats st2;
    ASSERT_EQ(OK, scan_ext4(&dev2, fl2, nullptr, nullptr, nullptr, tree2, ra2, rd2, nullptr, st2))
        << err_text(0);
    const tree::FileNode* gone_node = nullptr;
    std::function<void(const tree::FileNode*)> find2 = [&](const tree::FileNode* n) {
        for (const tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props() && c->name_utf8() == "Inode6")
                gone_node = c;
            find2(c);
        }
    };
    find2(ra2);
    find2(rd2);
    ASSERT_NE(nullptr, gone_node);
    ASSERT_EQ(size_t(1), gone_node->props()->extents.size());
    EXPECT_EQ(u64(1 * BS), gone_node->props()->extents[0].length)
        << "unwritten ee_len 0x8001 decodes to 1 block, not 0x8001*1024";
}

TEST(Ext4Engine, SizeExceedsBlocksSetsDamagedFlagConvergent) {
    /* N-177 (diff-to-fix): the S53 lane's CONVERGENT landing (the
     * cb2abd42 convergence kept both lanes' bodies; the shared name
     * broke the gtest registration - renamed, the 12c2f09e repair
     * class). The og's truncation-damage marker (node FLAGS
     * 0x200, EXT4SearchDLL :6300-6303 - FUN_180007510's
     * i_blocks < size>>9 verdict) lands as NODE_FLAG_SIZE_EXCEEDS_BLOCKS
     * (the same numeric bit 9), FILES only, clean 64-bit compute (the
     * og's +0x1F sign-extension on i_size_high NOT adopted - the
     * I-040/D-123 defect family). */
    auto img = build_volume();
    /* hello.txt: size 14, 1-block extent -> coverage 1024 >= 14: CLEAN.
     * Inflate the size beyond coverage to make it damaged. */
    u8* hello = img.block(B_INODE_TABLE) + size_t((INO_HELLO - 1) * INO_SZ);
    put32(hello + 0x04, 4096); /* i_size 4096 > coverage 1024 */
    /* gone (deleted file): size 512, coverage 1024: clean control in the
     * deleted path - then damage it too by zeroing coverage. */
    u8* gone = img.block(B_INODE_TABLE) + size_t((INO_GONE - 1) * INO_SZ);
    put32(gone + 0x1C, 0); /* i_blocks 0 < 512: damaged */

    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    ASSERT_EQ(OK, scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st))
        << err_text(0);

    const tree::FileNode* hello_node = nullptr;
    const tree::FileNode* sub_node = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (!c->props())
                continue;
            if (c->name_utf8() == "hello.txt")
                hello_node = c;
            if (c->name_utf8() == "sub")
                sub_node = c;
            find(c);
        }
    };
    find(ra);
    ASSERT_NE(nullptr, hello_node) << "the damaged file must still surface";
    ASSERT_NE(nullptr, sub_node) << "the clean dir must still surface";
    EXPECT_EQ(u16(tree::NODE_FLAG_SIZE_EXCEEDS_BLOCKS),
              u16(hello_node->props()->flags & tree::NODE_FLAG_SIZE_EXCEEDS_BLOCKS))
        << "i_size 4096 > i_blocks coverage 1024 -> the damaged bit set";
    EXPECT_EQ(u16(0), u16(sub_node->props()->flags & tree::NODE_FLAG_SIZE_EXCEEDS_BLOCKS))
        << "directories never carry the marker (the og's files-only gate)";

    /* the deleted path: gone.dat's zeroed coverage marks it too */
    ASSERT_NE(nullptr, rd->first_child());
    const tree::FileNode* gone_node = rd->first_child();
    EXPECT_EQ(u16(tree::NODE_FLAG_SIZE_EXCEEDS_BLOCKS),
              u16(gone_node->props()->flags & tree::NODE_FLAG_SIZE_EXCEEDS_BLOCKS))
        << "the deleted-inode path carries the same compute";
}

TEST(Ext4Engine, DamagedFlagCleanFilesCarryNoBit) {
    /* N-175-family regression pin: the pristine fixture (every inode's
     * coverage >= its size) sets the bit on NOTHING. */
    auto img = build_volume();
    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    ASSERT_EQ(OK, scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st))
        << err_text(0);
    std::function<void(tree::FileNode*)> walk = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props()) {
                EXPECT_EQ(u16(0), u16(c->props()->flags & tree::NODE_FLAG_SIZE_EXCEEDS_BLOCKS))
                    << "node " << c->name_utf8();
            }
            walk(c);
        }
    };
    walk(ra);
    walk(rd);
    walk(rl);
}

TEST(Ext4Engine, DamagedFlagSessionRoundTrip) {
    /* N-177: the marker persists - FileProperties::flags rides the
     * session record's put16/g16 pair (cr_records.cpp), so a save/load
     * cycle must preserve the bit. */
    auto img = build_volume();
    u8* hello = img.block(B_INODE_TABLE) + size_t((INO_HELLO - 1) * INO_SZ);
    put32(hello + 0x04, 4096); /* damaged: size beyond coverage */

    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    ASSERT_EQ(OK, scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st))
        << err_text(0);

    /* serialize the tree, reload it, compare */
    std::vector<u8> blob;
    ASSERT_EQ(OK, session::serialize_tree(tree, blob));
    tree::ResultTree tree2;
    ASSERT_EQ(OK, session::deserialize_tree(blob.data(), blob.size(), tree2));
    const tree::FileNode* hello2 = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props() && c->name_utf8() == "hello.txt")
                hello2 = c;
            find(c);
        }
    };
    for (auto* r : tree2.roots())
        find(r);
    ASSERT_NE(nullptr, hello2) << "the damaged file survives the round-trip";
    EXPECT_EQ(u16(tree::NODE_FLAG_SIZE_EXCEEDS_BLOCKS),
              u16(hello2->props()->flags & tree::NODE_FLAG_SIZE_EXCEEDS_BLOCKS))
        << "the damaged bit round-trips the session container";
}

TEST(Ext4Engine, JournalSalvage) {
    /* the fixture's jbd2 chain (BIG-endian, jsb type 3, descriptor with the
     * first-tag 16-byte UUID rule, ESCAPE-tagged block, matching commit)
     * must surface journal-evidenced names the live tree never saw */
    auto img = build_volume();
    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, nullptr, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(5u, st.journal_blocks);
    EXPECT_EQ(2u, st.journal_salvaged);
    std::set<std::string> lost;
    for (tree::FileNode* c = rl->first_child(); c; c = c->next_sibling()) {
        if (c->props())
            lost.insert(c->name_utf8());
    }
    EXPECT_TRUE(lost.count("ghost.txt"));
    EXPECT_TRUE(lost.count("esc.txt"));
    for (tree::FileNode* c = rl->first_child(); c; c = c->next_sibling()) {
        if (c->props() && c->name_utf8() == "esc.txt") {
            EXPECT_EQ(u64(0x98393BC0), c->props()->record_ref); /* escape restored */
        }
    }
}

TEST(Ext4Engine, InodeSizeOffsetIsSpecTrue) {
    /* s_inode_size is u16@0x58 (s_first_ino u32 owns 0x54..0x57): a SB with
     * a 256-byte inode size must NOT fall back to 128 */
    Img img(BPG);
    write_sb(img, 8);
    u8* sb = img.block(1);
    put16(sb + 0x58, 256);
    SuperBlock out;
    EXPECT_TRUE(validate_superblock(sb, out));
    EXPECT_EQ(256u, out.inode_size);
    /* a corrupt value falls back to 128 */
    put16(sb + 0x58, 12);
    EXPECT_TRUE(validate_superblock(sb, out));
    EXPECT_EQ(128u, out.inode_size);
}

TEST(Ext4Engine, JournalUncommittedTailSurfaces) {
    /* E-2 (AUDIT-LINUX-FS, D-07 re-pin): the og feeds EVERY journal block
     * to the dir parser with NO commit/transaction gating
     * (EXT4SearchDLL FUN_18000e5b0/e890) - an uncommitted tail's dir
     * blocks surface. Our old commit gate dropped them. Extend the journal
     * with a descriptor (sequence 10) that has NO commit block. */
    auto img = build_volume();
    constexpr u64 B_JOURNAL = 10;
    /* grow the journal inode by 2 blocks (5 -> 7) and plant the tail */
    write_inode(img, 8, 0x8000, 7 * BS, B_JOURNAL, 7, 1, 0);
    bput32(img.block(B_JOURNAL) + 0x10, 7); /* s_maxlen covers the tail */
    {
        u8* desc = img.block(B_JOURNAL + 5);
        bput32(desc + 0, 0xC03B3998u);
        bput32(desc + 4, 1);   /* JBD2_DESCRIPTOR_BLOCK */
        bput32(desc + 8, 10);  /* sequence 10 - NEVER committed */
        bput32(desc + 12, 40); /* tag: fs block (value unused by parser) */
        bput16(desc + 16, 0);
        bput16(desc + 18, 8); /* LAST_TAG (no UUID games: SAME_UUID set) */
        u8* snap = img.block(B_JOURNAL + 6);
        u32 jcur = 0;
        add_dirent(snap, jcur, 51, "uncommitted.txt", 1);
    }
    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, nullptr, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(7u, st.journal_blocks);
    EXPECT_EQ(3u, st.journal_salvaged); /* ghost + esc + UNCOMMITTED tail */
    bool saw = false;
    for (tree::FileNode* c = rl->first_child(); c; c = c->next_sibling()) {
        if (c->props() && c->name_utf8() == "uncommitted.txt")
            saw = true;
    }
    EXPECT_TRUE(saw) << "uncommitted-tail dir block must surface (og parity)";
}

TEST(Ext4Engine, FamilyKindStampsPerDecodedAnalyserRule) {
    /* OQ-S52-1 (resolved): the og's ext-family classification - the
     * analyser's verdict rule decoded first-hand (ExtFileSystemAnalyser
     * :3616-3680): the strict ext4 shape (incompat EXTENTS|64BIT|FLEX_BG
     * == 0x2c0) is kind 9; the pre-ext4 family (incompat within
     * {FILETYPE, HAS_JOURNAL, META_BG}, ro_compat within {SPARSE_SUPER,
     * LARGE_FILE, DIR_INDEX}) is the family code 4 - ext2 and ext3 BOTH
     * (the engine space collapses the analyser's verdicts 5/6); shapes
     * the og's analyser refuses keep the ext4 stamp (no og behavior). */
    SuperBlock sb;
    u8 raw[1024];

    /* the family shape: journal set (ext3-class), no extents */
    {
        Img img(BPG);
        write_sb(img, 8);
        put32(img.block(1) + 0x60, 0x0004); /* s_feature_incompat: HAS_JOURNAL */
        memcpy(raw, img.block(1), 1024);
        ASSERT_TRUE(validate_superblock(raw, sb));
        EXPECT_EQ(u32(FsKindCode::Ext2), u32(ext_family_kind(sb)))
            << "a journaled family shape stamps the family code (ext3 -> kind 4)";
    }
    /* the family shape: journal clear (ext2-class) */
    {
        Img img(BPG);
        write_sb(img, 8);
        put32(img.block(1) + 0x60, 0x0002); /* s_feature_incompat: FILETYPE only */
        memcpy(raw, img.block(1), 1024);
        ASSERT_TRUE(validate_superblock(raw, sb));
        EXPECT_EQ(u32(FsKindCode::Ext2), u32(ext_family_kind(sb)))
            << "a journal-less family shape stamps the family code (ext2 -> kind 4)";
    }
    /* the strict ext4 shape: all three of EXTENTS|64BIT|FLEX_BG */
    {
        Img img(BPG);
        write_sb(img, 8);
        put32(img.block(1) + 0x60, 0x02c0); /* EXTENTS|64BIT|FLEX_BG */
        memcpy(raw, img.block(1), 1024);
        ASSERT_TRUE(validate_superblock(raw, sb));
        EXPECT_EQ(u32(FsKindCode::Ext4), u32(ext_family_kind(sb)))
            << "the 0x2c0 shape stays kind 9 (unshifted)";
    }
    /* a shape the og's analyser refuses (EXTENTS set, 64BIT clear):
     * keeps the ext4 stamp - no og shift behavior exists for it */
    {
        Img img(BPG);
        write_sb(img, 8);
        put32(img.block(1) + 0x60, 0x0042); /* EXTENTS+FILETYPE, no 64BIT/FLEX_BG */
        memcpy(raw, img.block(1), 1024);
        ASSERT_TRUE(validate_superblock(raw, sb));
        EXPECT_EQ(u32(FsKindCode::Ext4), u32(ext_family_kind(sb)))
            << "the og-refused shape keeps the unshifted stamp";
    }
}

} // namespace
} // namespace ext4
} // namespace cr

namespace cr {
namespace ext4 {

/* ---- S-004: every-block journal salvage + jsb gate --------------------- */

TEST(Ext4Engine, JournalUnreferencedDirBlockSalvaged) {
    /* S-004 (diff-to-fix): the og feeds EVERY block of the declared log
     * range to the dir parser - a directory remnant no surviving
     * descriptor references (overwritten ring, mid-write tail) surfaces.
     * The old descriptor-referenced-only salvage dropped it. */
    auto img = build_volume();
    constexpr u64 B_JOURNAL = 10;
    write_inode(img, 8, 0x8000, 8 * BS, B_JOURNAL, 8, 1, 0);
    bput32(img.block(B_JOURNAL) + 0x10, 8); /* s_maxlen: whole range */
    {
        /* a dir block at journal block 6 that NO descriptor tags */
        u8* rem = img.block(B_JOURNAL + 6);
        u32 cur = 0;
        add_dirent(rem, cur, 52, "orphan.txt", 1);
        /* a plain junk block at 7 - must not parse, must not crash */
        memset(img.block(B_JOURNAL + 7), 0x11, BS);
    }
    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, nullptr, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(8u, st.journal_blocks);
    EXPECT_EQ(3u, st.journal_salvaged); /* ghost + esc + UNREFERENCED */
    bool saw = false;
    for (tree::FileNode* c = rl->first_child(); c; c = c->next_sibling())
        if (c->props() && c->name_utf8() == "orphan.txt")
            saw = true;
    EXPECT_TRUE(saw) << "unreferenced journal dir block must surface (og parity)";
}

TEST(Ext4Engine, JournalFirstBlockNotJsbNoSalvage) {
    /* S-004 strict parity: the og refuses the whole salvage when journal
     * block 0 is not a superblock (magic + blocktype {3,4} gate,
     * :12207/:12212). A descriptor-headed journal yields nothing on both
     * sides now (the pre-fix core salvaged it - a superset the audit
     * flagged as claim 2). */
    auto img = build_volume();
    constexpr u64 B_JOURNAL = 10;
    /* turn the jsb into a descriptor block: same magic, type 1 */
    bput32(img.block(B_JOURNAL) + 4, 1); /* JBD2_DESCRIPTOR_BLOCK */
    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, nullptr, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(5u, st.journal_blocks);   /* the journal data still counts */
    EXPECT_EQ(0u, st.journal_salvaged); /* but nothing is salvaged */
    for (tree::FileNode* c = rl->first_child(); c; c = c->next_sibling())
        if (c->props())
            ADD_FAILURE() << "no journal node may surface without a jsb";
}

/* ===================================================================== */
/* M-037 (diff-to-fix): the og's dir-block damage model — decoded from   */
/* EXT4SearchDLL FUN_180005820 (:4146-4168): invalid entries SKIP by a   */
/* 4-byte resync slide (entries after a corruption survive); type must   */
/* be 1..7; rec_len >= (name_len+11)&~3; inode# <= inodes_count. The     */
/* core's deliberate superset: walking PAST inode-0 holes (og stops).    */
/* ===================================================================== */

TEST(Ext4Engine, CorruptRecLenSkipsEntryNotBlock) {
    /* "hello.txt" sits at root-block offset 24; corrupting its rec_len
     * must cost ONLY that entry — "sub" (at 44) and its nested file must
     * survive the 4-byte resync slide. Fail-before: the old break dropped
     * every entry after the corruption. */
    auto img = build_volume();
    u8* root = img.block(B_ROOTDIR);
    put16(root + 24 + 4, 0x00FF); /* misaligned rec_len on hello.txt */
    MemDevice dev(img.raw());

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);

    std::set<std::string> found;
    std::function<void(tree::FileNode*, const std::string&)> collect = [&](tree::FileNode* n,
                                                                           const std::string& p) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (!c->props())
                continue;
            const std::string nm = c->name_utf8();
            const std::string full = p.empty() ? nm : (p == "/" ? "/" + nm : p + "/" + nm);
            found.insert(full);
            collect(c, full);
        }
    };
    collect(ra, "");
    EXPECT_FALSE(found.count("/hello.txt")) << "the corrupt entry itself is lost";
    EXPECT_TRUE(found.count("/sub")) << "entries after the corruption must survive";
    EXPECT_TRUE(found.count("/sub/nested.bin"));
}

TEST(Ext4Engine, InodeZeroHoleContinues) {
    /* Deliberate superset PIN (M-037): the og's walk STOPS at an inode-0
     * hole (FUN_180005820 :4155); the core continues past it, recovering
     * the live entries that follow. Rebuild the root dir with a hole
     * between ".." and the live files. */
    auto img = build_volume();
    u8* root = img.block(B_ROOTDIR);
    memset(root, 0, BS);
    u32 cur = 0;
    add_dirent(root, cur, INO_ROOT, ".", 2);
    add_dirent(root, cur, INO_ROOT, "..", 2);
    add_dirent(root, cur, 0, "hole-entry", 1); /* the og stops here */
    add_dirent(root, cur, INO_HELLO, "hello.txt", 1);
    add_dirent(root, cur, INO_DIR, "sub", 2);
    MemDevice dev(img.raw());

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);

    std::set<std::string> found;
    std::function<void(tree::FileNode*, const std::string&)> collect = [&](tree::FileNode* n,
                                                                           const std::string& p) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (!c->props())
                continue;
            const std::string nm = c->name_utf8();
            const std::string full = p.empty() ? nm : (p == "/" ? "/" + nm : p + "/" + nm);
            found.insert(full);
            collect(c, full);
        }
    };
    collect(ra, "");
    EXPECT_TRUE(found.count("/hello.txt")) << "past-hole live entries survive (our superset)";
    EXPECT_TRUE(found.count("/sub"));
    EXPECT_TRUE(found.count("/sub/nested.bin"));
}

TEST(Ext4Engine, FileTypeGateSkipsInvalid) {
    /* The og's type gate (1..7) skips entries whose type byte is invalid
     * without dropping the rest of the block. Fail-before: no type gate
     * existed, so a type-8 entry was emitted as a file. */
    auto img = build_volume();
    u8* root = img.block(B_ROOTDIR);
    memset(root, 0, BS);
    u32 cur = 0;
    add_dirent(root, cur, INO_ROOT, ".", 2);
    add_dirent(root, cur, INO_ROOT, "..", 2);
    add_dirent(root, cur, INO_HELLO, "badtype8.txt", 8); /* EXT4_FT max is 7 */
    add_dirent(root, cur, INO_DIR, "sub", 2);
    MemDevice dev(img.raw());

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);

    std::set<std::string> found;
    std::function<void(tree::FileNode*, const std::string&)> collect = [&](tree::FileNode* n,
                                                                           const std::string& p) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (!c->props())
                continue;
            const std::string nm = c->name_utf8();
            const std::string full = p.empty() ? nm : (p == "/" ? "/" + nm : p + "/" + nm);
            found.insert(full);
            collect(c, full);
        }
    };
    collect(ra, "");
    EXPECT_FALSE(found.count("/badtype8.txt")) << "type-8 entries are skipped (og gate 1..7)";
    EXPECT_TRUE(found.count("/sub")) << "the block survives the skipped entry";
    EXPECT_TRUE(found.count("/sub/nested.bin"));
}

/* ---- S-005: on-disk UTF-8 names decode, never byte-lift ---------------- */

TEST(Ext4Engine, NonAsciiNamesDecodedNotByteLifted) {
    /* ext4 directory names are raw UTF-8 bytes; the og passes them through
     * (EXT4SearchDLL imports no conversion API; .rsf stores UTF-8). The
     * engine must decode them to true code points - the pre-fix byte-lift
     * turned "照片" into "æ–‡ä»¶"-style mojibake. */
    auto img = build_volume();
    {
        /* append entries after the existing root-dir records (cursor 76):
         * CJK "照片.jpg", accent "café.txt", and a torn sequence name */
        u8* root = img.block(B_ROOTDIR);
        u32 cur = 76;
        add_dirent(root, cur, INO_HELLO, "\xe7\x85\xa7\xe7\x89\x87.jpg", 1);
        add_dirent(root, cur, INO_SUB, "caf\xc3\xa9.txt", 1);
        add_dirent(root, cur, INO_GONE, "\xff(bad", 1);
    }
    MemDevice dev(img.raw());

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);

    std::map<std::string, std::u16string> by_utf8;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props())
                by_utf8[c->name_utf8()] = c->props()->name;
            collect(c);
        }
    };
    collect(ra);

    /* CJK name: true code points, not Latin-1-per-byte mojibake */
    ASSERT_TRUE(by_utf8.count("\xe7\x85\xa7\xe7\x89\x87.jpg") != 0)
        << "CJK name absent from results";
    EXPECT_EQ(by_utf8["\xe7\x85\xa7\xe7\x89\x87.jpg"], std::u16string(u"\u7167\u7247.jpg"));
    /* accented name */
    ASSERT_TRUE(by_utf8.count("caf\xc3\xa9.txt") != 0);
    EXPECT_EQ(by_utf8["caf\xc3\xa9.txt"], std::u16string(u"caf\u00e9.txt"));
    /* M-010 (diff-to-fix): an invalid lead byte now maps LOSSLESSLY to
     * U+E000 + byte (the og's EXT4 engine keeps raw names unconverted -
     * zero conversion imports; the original bytes must stay
     * reconstructible: unit - 0xE000 = 0xFF). The valid tail survives;
     * the map key is the tree's UTF-8 projection of U+E0FF (EE 83 BF). */
    ASSERT_TRUE(by_utf8.count("\xee\x83\xbf(bad") != 0)
        << "lossless name absent: " << by_utf8.begin()->first;
    EXPECT_EQ(by_utf8["\xee\x83\xbf(bad"], std::u16string(u"\uE0FF(bad"));
    /* whole-pipeline: the UTF-8 projection of a decoded CJK name equals
     * the original on-disk bytes (this is what tree --json emits) */
    for (tree::FileNode* c = ra->first_child(); c; c = c->next_sibling()) {
        for (tree::FileNode* f = c->first_child(); f; f = f->next_sibling()) {
            if (f->props() && f->props()->name == std::u16string(u"\u7167\u7247.jpg")) {
                EXPECT_STREQ("\xe7\x85\xa7\xe7\x89\x87.jpg", f->name_utf8().c_str());
            }
        }
    }
}

/* ===================================================================== */
/* S-026: meta_bg group-descriptor resolution.                           */
/*                                                                       */
/* The og resolves meta-group descriptors only for geometries where      */
/* s_desc_size^2 == block_size (FUN_180004c70 :3526 stride = s_desc_size */
/* at :3368-3373; read at m*blocks_per_group*block_size :3582-3583,      */
/* asm-verified) and reads one block early on first_data_block == 1       */
/* volumes. This suite implements the kernel/e2fsprogs-correct layout,   */
/* verified this session on real mke2fs images (1K/64B and 4K/64B):      */
/*  - meta_group_size = block_size / desc_size                           */
/*  - descriptor block for meta-group m at                               */
/*    first_data_block + m*meta_group_size*bpg (+1 only for m == 0)      */
/*  - meta-groups below s_first_meta_bg keep the classic front blocks    */
/*    first_data_block + 1 + m                                           */
/* ===================================================================== */

namespace {

constexpr u32 MBG_BPG = 64;    /* blocks per group (synthetic, small) */
constexpr u32 MBG_IPG = 16;    /* inodes per group */
constexpr u32 MBG_GROUPS = 32; /* groups 0..31 -> meta-groups 0 and 1 */
constexpr u32 MBG_META = 16;   /* 1024 / 64 descriptors per meta-group */
constexpr u64 MBG_BLOCKS = u64(MBG_GROUPS) * MBG_BPG;

/* meta-group 0 descriptor block = first_data_block + 1 = block 2 */
constexpr u64 MBG_FRONT_DESC = 2;
/* group 16's first block = first_data_block + 16*bpg = 1 + 1024 */
constexpr u64 MBG_G16_DESC = 1 + 16 * MBG_BPG;
constexpr u64 MBG_G16_ITABLE = MBG_G16_DESC + 1; /* right after the desc block */
constexpr u64 MBG_G16_BITMAP = MBG_G16_DESC + 3;
constexpr u64 MBG_ROOTDIR = 6;
constexpr u64 MBG_HELLO_DATA = 7;
constexpr u64 MBG_FAR_DATA = MBG_G16_DESC + 4;

constexpr u32 MBG_INO_FAR = 16 * MBG_IPG + 1; /* first inode of group 16 */

/* write an inode into the table of ITS OWN group (tables at different
 * blocks per group) */
void mbg_write_inode(Img& img, u32 ino, u16 mode, u64 size, u64 extent_block, u32 extent_blocks,
                     u16 links, u32 dtime, u64 itable_block) {
    u8* p = img.block(itable_block) + size_t((ino - 1) % MBG_IPG) * INO_SZ;
    memset(p, 0, INO_SZ);
    put16(p + 0x00, mode);
    put32(p + 0x04, u32(size));
    put32(p + 0x08, 1000);
    put32(p + 0x0C, 2000);
    put32(p + 0x10, 3000);
    put32(p + 0x14, dtime);
    put16(p + 0x1A, links);
    /* N-177: i_blocks_lo (512-byte units) - real volumes always carry
     * the coverage word; the fixture now reflects that. */
    put32(p + 0x1C, u32(u64(extent_blocks) * (BS / 512)));
    put32(p + 0x20, 0x80000); /* EXTENTS_FL */
    u8* ib = p + 0x28;
    put16(ib + 0, 0xF30A);
    put16(ib + 2, 1);
    put16(ib + 4, 4);
    put16(ib + 6, 0);
    put32(ib + 8, 0);
    put32(ib + 12, 0);
    put16(ib + 16, u16(extent_blocks));
    put16(ib + 18, 0);
    put32(ib + 20, u32(extent_block));
}

/* meta_bg superblock: 64bit + meta_bg + filetype; desc_size 64 */
void mbg_write_sb(Img& img, u32 first_meta_bg) {
    u8* sb = img.block(1);
    memset(sb, 0, BS);
    put32(sb + 0x00, MBG_GROUPS * MBG_IPG); /* s_inodes_count */
    put32(sb + 0x04, u32(MBG_BLOCKS));      /* s_blocks_count_lo */
    put32(sb + 0x14, 1);                    /* s_first_data_block */
    put32(sb + 0x18, 0);                    /* log_block_size 0 (1024) */
    put32(sb + 0x20, MBG_BPG);              /* s_blocks_per_group */
    put32(sb + 0x28, MBG_IPG);              /* s_inodes_per_group */
    put16(sb + 0x38, 0xEF53);
    put32(sb + 0x4C, 1); /* rev level 1 */
    put16(sb + 0x58, INO_SZ);
    put32(sb + 0x60, 0x0092);         /* filetype | meta_bg | 64bit */
    put16(sb + 0xFE, 64);             /* s_desc_size */
    put32(sb + 0x230, first_meta_bg); /* s_first_meta_bg */
    put32(sb + 0xE0, 8);
}

Img build_metabg_volume(u32 first_meta_bg) {
    Img img(MBG_BLOCKS);
    mbg_write_sb(img, first_meta_bg);

    /* meta-group 0 descriptor block (front, group 0's start + 1):
     * only group 0's descriptor is populated; groups 1..15 unused */
    u8* d0 = img.block(MBG_FRONT_DESC);
    memset(d0, 0, BS);
    put32(d0 + 0x00, u32(MBG_G16_BITMAP)); /* group 0 block bitmap (any) */
    put32(d0 + 0x08, 3);                   /* group 0 inode table at 3 */

    if (first_meta_bg == 0) {
        /* distributed layout: meta-group 1's descriptor block IS group 16's
         * first block (no backup SB - empirically verified) */
        u8* d1 = img.block(MBG_G16_DESC);
        memset(d1, 0, BS);
        put32(d1 + 0x00, u32(MBG_G16_BITMAP)); /* group 16 block bitmap */
        put32(d1 + 0x08, u32(MBG_G16_ITABLE)); /* group 16 inode table */
        /* 64-bit high words (desc_size 64): inode_table_hi@+0x28 */
        put32(d1 + 0x28, 0);
    } else {
        /* classic front region (s_first_meta_bg >= 2): meta-group 1's
         * descriptors at first_data_block + 1 + 1 (the second front block) */
        u8* d1 = img.block(MBG_FRONT_DESC + 1);
        memset(d1, 0, BS);
        put32(d1 + 0x00, u32(MBG_G16_BITMAP));
        put32(d1 + 0x08, u32(MBG_G16_ITABLE));
        put32(d1 + 0x28, 0);
    }

    /* inodes: group 0 table at block 3 (2 blocks for 16 x 128B) */
    mbg_write_inode(img, INO_ROOT, 0x4000, BS, MBG_ROOTDIR, 1, 2, 0, 3);
    mbg_write_inode(img, INO_HELLO, 0x8000, 14, MBG_HELLO_DATA, 1, 1, 0, 3);
    mbg_write_inode(img, 8, 0x8000, 0, 0, 0, 1, 0, 3); /* journal: empty */
    /* group 16 table: the far inode lives BEYOND the front GDT reach */
    mbg_write_inode(img, MBG_INO_FAR, 0x8000, 33, MBG_FAR_DATA, 1, 1, 0, MBG_G16_ITABLE);

    /* root dir: ".", "..", hello.txt, far.bin */
    u8* root = img.block(MBG_ROOTDIR);
    memset(root, 0, BS);
    u32 cur = 0;
    add_dirent(root, cur, INO_ROOT, ".", 2);
    add_dirent(root, cur, INO_ROOT, "..", 2);
    add_dirent(root, cur, INO_HELLO, "hello.txt", 1);
    add_dirent(root, cur, MBG_INO_FAR, "far.bin", 1);

    memcpy(img.block(MBG_HELLO_DATA), "meta-front", 11);
    memset(img.block(MBG_FAR_DATA), 0x5A, 33);
    return img;
}

/* device that counts reads at a given byte range (cache-reuse probe) */
class CountingDevice final : public MemDevice {
public:
    CountingDevice(std::vector<u8> d, u64 watch_off) : MemDevice(std::move(d)), watch_(watch_off) {}
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off == watch_)
            ++hits_;
        return MemDevice::read_at(off, buf, len);
    }
    u64 hits() const { return hits_; }

private:
    u64 watch_ = 0;
    u64 hits_ = 0;
};

std::set<std::string> collect_names(tree::FileNode* n) {
    std::set<std::string> out;
    std::function<void(tree::FileNode*)> walk = [&](tree::FileNode* p) {
        for (tree::FileNode* c = p->first_child(); c; c = c->next_sibling()) {
            if (c->props())
                out.insert(c->name_utf8());
            walk(c);
        }
    };
    walk(n);
    return out;
}

} // namespace

TEST(Ext4Engine, MetaBgVolumeRecoversAllGroups) {
    /* files whose inodes live in a group served by a DISTRIBUTED
     * meta-group descriptor block must resolve: the pre-fix engine read the
     * whole "GDT" from the front block, so group 16's descriptor came from
     * bitmap bytes and far.bin silently vanished */
    auto img = build_metabg_volume(0);
    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(32u, st.block_groups);

    const auto found = collect_names(ra);
    EXPECT_TRUE(found.count("hello.txt")) << "front-group file missing";
    ASSERT_TRUE(found.count("far.bin")) << "meta-group-1 file missing (S-026)";
}

TEST(Ext4Engine, MetaBgDeletedSweepReachesDistributedGroups) {
    /* the deleted-inode sweep resolves every group too: an inode in group
     * 16 with dtime set must be emitted under Deleted (pre-fix: silently
     * missing for the same front-GDT reason) */
    auto img = build_metabg_volume(0);
    mbg_write_inode(img, MBG_INO_FAR + 1, 0x8000, 64, MBG_FAR_DATA + 1, 1, 0, 4242, MBG_G16_ITABLE);
    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, nullptr, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    const auto deleted = collect_names(rd);
    EXPECT_NE(0u, deleted.size());
    bool has_far_deleted = false;
    for (const auto& n : deleted)
        if (n.find("Inode258") == 0)
            has_far_deleted = true;
    EXPECT_TRUE(has_far_deleted) << "group-16 deleted inode missing (S-026)";
}

TEST(Ext4Engine, MetaBgDescriptorBlockReadOnce) {
    /* per-meta-group cache: repeated group-16 inode reads must hit the ONE
     * descriptor-block read, not re-read it (the og caches per meta-group
     * :3664-3703; the perf-guard twin of that behavior) */
    auto img = build_metabg_volume(0);
    /* a second dirent in group 16 forces multiple inode reads there */
    mbg_write_inode(img, MBG_INO_FAR + 1, 0x8000, 8, MBG_FAR_DATA + 1, 1, 1, 0, MBG_G16_ITABLE);
    u8* root = img.block(MBG_ROOTDIR);
    u32 cur = 76;
    add_dirent(root, cur, MBG_INO_FAR + 1, "far2.bin", 1);
    CountingDevice dev(img.raw(), MBG_G16_DESC * BS);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, nullptr, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(1u, dev.hits()) << "meta-group descriptor block re-read";
}

TEST(Ext4Engine, MetaBgFirstMetaBgFrontRegion) {
    /* s_first_meta_bg = 2: descriptor blocks 0 AND 1 live in the classic
     * front region (first_data_block + 1 + m for m < 2), per e2fsprogs
     * ext2fs_descriptor_block_loc2's i < s_first_meta_bg branch; with
     * first_meta_bg = 1 meta-group 1 is already distributed, so 2 is the
     * minimal probe of the front-region branch */
    auto img = build_metabg_volume(2);
    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, nullptr, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    const auto found = collect_names(ra);
    EXPECT_TRUE(found.count("hello.txt"));
    ASSERT_TRUE(found.count("far.bin")) << "front-region meta-group unresolved";
}

TEST(Ext4Engine, MetaBgSuperblockFieldsParsed) {
    auto img = build_metabg_volume(0);
    SuperBlock sb;
    ASSERT_TRUE(validate_superblock(img.block(1), sb));
    EXPECT_EQ(0u, sb.first_meta_bg);
    EXPECT_NE(0u, sb.feature_incompat & 0x10);
    EXPECT_EQ(64u, sb.desc_size);
    put32(img.block(1) + 0x230, 2);
    ASSERT_TRUE(validate_superblock(img.block(1), sb));
    EXPECT_EQ(2u, sb.first_meta_bg);
}

/* ===================================================================== */
/* S-027: backup-superblock rescue (the og's engine-side lost-volume     */
/* walk: FUN_180001b00 + sub-searcher spawn, corpus artifact §2).        */
/* ===================================================================== */

namespace {

/* canonical 1KiB geometry: 2 groups, so the group-1 backup superblock
 * sits at block first_data_block + 1*8192 = 8193 = 8 MiB + 1024 with
 * s_block_group_nr == 1 (verified on real mke2fs images this session) */
constexpr u32 BKB_BPG = 8192;
constexpr u32 BKB_IPG = 16;
constexpr u64 BKB_BLOCKS = 2 * BKB_BPG + 1; /* blocks 0..16384 */
constexpr u64 BKB_BACKUP_SB = 8193;
constexpr u64 BKB_BACKUP_GDT = 8194;
constexpr u64 BKB_G1_ITABLE = 8195;
constexpr u64 BKB_G1_BITMAP = 8198;
constexpr u64 BKB_FAR_DATA = 8197;
constexpr u32 BKB_INO_FAR = 1 * BKB_IPG + 1; /* inode 17: group 1 */

Img build_backup_volume(bool kill_primary_gdt) {
    Img img(BKB_BLOCKS);
    /* primary superblock (block 1) - will be zeroed after copying */
    u8* sb = img.block(1);
    put32(sb + 0x00, 2 * BKB_IPG);     /* s_inodes_count */
    put32(sb + 0x04, u32(BKB_BLOCKS)); /* s_blocks_count_lo */
    put32(sb + 0x14, 1);               /* s_first_data_block */
    put32(sb + 0x18, 0);               /* log_block_size 0 (1024) */
    put32(sb + 0x20, BKB_BPG);         /* s_blocks_per_group */
    put32(sb + 0x28, BKB_IPG);         /* s_inodes_per_group */
    put16(sb + 0x38, 0xEF53);
    put32(sb + 0x4C, 1); /* rev level 1 */
    put16(sb + 0x58, INO_SZ);
    put32(sb + 0x60, 0x0002); /* FILETYPE */
    put32(sb + 0xE0, 8);
    /* backup superblock at block 8193: same fields + s_block_group_nr = 1
     * (the real mkfs backup shape - verified: primary 0, group-1 copy 1) */
    u8* bsb = img.block(BKB_BACKUP_SB);
    memcpy(bsb, sb, 1024);
    put16(bsb + 0x5A, 1); /* s_block_group_nr */

    /* primary GDT at block 2: group 0 + group 1 descriptors */
    u8* g = img.block(2);
    memset(g, 0, BS);
    put32(g + 0x00, 5); /* group 0 block bitmap */
    put32(g + 0x04, 0);
    put32(g + 0x08, 3); /* group 0 inode table */
    put32(g + 32 + 0x00, u32(BKB_G1_BITMAP));
    put32(g + 32 + 0x08, u32(BKB_G1_ITABLE));
    /* backup GDT at block 8194: the full table copy (e2fsprogs layout) */
    memcpy(img.block(BKB_BACKUP_GDT), g, 64);

    if (kill_primary_gdt)
        memset(img.block(2), 0, BS); /* front GDT dead */

    /* group 0 inodes at blocks 3..4 */
    mbg_write_inode(img, INO_ROOT, 0x4000, BS, 6, 1, 2, 0, 3);
    mbg_write_inode(img, INO_HELLO, 0x8000, 14, 7, 1, 1, 0, 3);
    mbg_write_inode(img, 8, 0x8000, 0, 0, 0, 1, 0, 3); /* journal: empty */
    /* group 1 inode table at blocks 8195..8196: the far inode */
    mbg_write_inode(img, BKB_INO_FAR, 0x8000, 33, BKB_FAR_DATA, 1, 1, 0, BKB_G1_ITABLE);

    /* root dir: ".", "..", hello.txt, far.bin */
    u8* root = img.block(6);
    memset(root, 0, BS);
    u32 cur = 0;
    add_dirent(root, cur, INO_ROOT, ".", 2);
    add_dirent(root, cur, INO_ROOT, "..", 2);
    add_dirent(root, cur, INO_HELLO, "hello.txt", 1);
    add_dirent(root, cur, BKB_INO_FAR, "far.bin", 1);

    memcpy(img.block(7), "backup-rescue", 14);
    memset(img.block(BKB_FAR_DATA), 0x66, 33);

    /* destroy the primary superblock last */
    memset(img.block(1), 0, BS);
    return img;
}

} // namespace

TEST(Ext4Engine, BackupSuperblockRescueWhenPrimaryDestroyed) {
    /* the finding's core scenario: primary SB destroyed, group-1 backup
     * alive. The pre-fix engine returned ERR_BAD_MAGIC and the volume was
     * invisible; the og's stride walker + sub-searcher found it. */
    auto img = build_backup_volume(false);
    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(2u, st.block_groups);
    EXPECT_EQ(8193u, st.backup_sb_block);
    EXPECT_FALSE(st.gdt_from_backup) << "front GDT alive: no fallback";

    const auto found = collect_names(ra);
    EXPECT_TRUE(found.count("hello.txt"));
    ASSERT_TRUE(found.count("far.bin")) << "group-1 file missing through rescue";
}

TEST(Ext4Engine, BackupRescueFallsBackToBackupGdt) {
    /* primary SB AND front GDT dead: descriptors must come from the backup
     * GDT immediately after the found backup SB (e2fsprogs "Backup
     * superblock at B, Group descriptors at B+1" - verified on real
     * images: classic volumes carry the FULL table there) */
    auto img = build_backup_volume(true);
    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_TRUE(st.gdt_from_backup);
    const auto found = collect_names(ra);
    ASSERT_TRUE(found.count("far.bin")) << "backup-GDT resolution failed";
}

TEST(Ext4Engine, BackupRescueWholeDiskRebase) {
    /* the volume embedded at a 1 MiB-aligned offset inside a bigger
     * container (the whole-disk dispatch case): the backup SB is found at
     * container + 8 MiB + 1024; s_block_group_nr arithmetic yields the
     * exact volume base, and emitted extents are container-relative */
    constexpr u64 VOL_OFF = 2ull << 20; /* 2 MiB (grid-aligned) */
    auto img = build_backup_volume(false);
    std::vector<u8> disk(size_t(VOL_OFF) + img.size(), 0);
    memcpy(disk.data() + VOL_OFF, img.raw().data(), img.size());
    MemDevice dev(disk);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(8193u, st.backup_sb_block); /* volume-relative block, not byte */
    const auto found = collect_names(ra);
    ASSERT_TRUE(found.count("far.bin")) << "rebased volume unresolved";
    /* emitted extents are SLICE-relative (base included) */
    bool checked = false;
    for (tree::FileNode* c = ra->first_child(); c && !checked; c = c->next_sibling()) {
        for (tree::FileNode* f = c->first_child(); f; f = f->next_sibling()) {
            if (f->props() && f->name_utf8() == "far.bin") {
                ASSERT_FALSE(f->props()->extents.empty());
                EXPECT_EQ(VOL_OFF + BKB_FAR_DATA * BS, f->props()->extents[0].offset);
                checked = true;
                break;
            }
        }
    }
    ASSERT_TRUE(checked) << "far.bin node not found for extent check";
}

TEST(Ext4Engine, BadMagicWithoutBackupsStillRejected) {
    /* rescue finds nothing (backup SB destroyed too): the original
     * ERR_BAD_MAGIC contract stands - probes ran and found no candidate */
    auto img = build_backup_volume(false);
    memset(img.block(BKB_BACKUP_SB), 0, BS);
    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st);
    EXPECT_EQ(ERR_BAD_MAGIC, rc);
    EXPECT_NE(0u, st.backup_sb_probes) << "probe grid never ran";
    EXPECT_EQ(0u, st.backup_sb_block);
}

} // namespace ext4
} // namespace cr

/* ---- S-025: journal-salvaged reparenting (the og's two-pass model) ------ */

namespace cr {
namespace ext4 {
namespace {

/* extend the fixture's journal to `total` blocks and rewrite the jsb */
void grow_journal(Img& img, u32 total) {
    constexpr u64 B_JOURNAL = 10;
    write_inode(img, 8 /* journal inum per SB 0xE0 */, 0x8000, total * BS, B_JOURNAL, total, 1, 0);
    u8* jsb = img.block(B_JOURNAL);
    bput32(jsb + 0x10, total); /* s_maxlen - the declared log range */
}

} // namespace

TEST(Ext4Engine, JournalSalvagedDirReparented) {
    /* a deleted directory whose journal copy survives: the journal holds a
     * snapshot of the LIVE parent's dir block (naming the deleted dir) AND
     * the deleted dir's own block (naming its child). The og reparents
     * the salvaged dir under its original parent (FUN_180003de0's orphan
     * finalize :3077-3119); ours must too - not flat in Lost Files. */
    auto img = build_volume();
    grow_journal(img, 8);
    /* journal block 5: a snapshot of "sub" (inode 4) with an EXTRA entry
     * naming the deleted dir "childdir" (inode 7) */
    u8* snap_sub = img.block(10 + 5);
    memset(snap_sub, 0, BS);
    u32 c1 = 0;
    add_dirent(snap_sub, c1, 4, ".", 2);          /* owner: sub (LIVE) */
    add_dirent(snap_sub, c1, 2, "..", 2);         /* sub's parent: root */
    add_dirent(snap_sub, c1, 5, "nested.bin", 1); /* live child */
    add_dirent(snap_sub, c1, 7, "childdir", 2);   /* deleted dir */
    /* journal block 6: childdir's own snapshot: owner 7, parent 4 (sub) */
    u8* snap_child = img.block(10 + 6);
    memset(snap_child, 0, BS);
    u32 c2 = 0;
    add_dirent(snap_child, c2, 7, ".", 2);
    add_dirent(snap_child, c2, 4, "..", 2);
    add_dirent(snap_child, c2, 9, "deep.txt", 1);

    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, nullptr, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);

    /* find the LIVE "sub" node; "childdir" must be ITS child, and
     * "deep.txt" must sit under "childdir" - nothing in Lost Files */
    std::function<tree::FileNode*(tree::FileNode*, const char*)> find =
        [&](tree::FileNode* n, const char* name) -> tree::FileNode* {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props() && c->name_utf8() == name)
                return c;
            if (tree::FileNode* r = find(c, name))
                return r;
        }
        return nullptr;
    };
    tree::FileNode* sub = find(ra, "sub");
    ASSERT_NE(sub, nullptr) << "live sub dir must exist";
    tree::FileNode* childdir = find(sub, "childdir");
    ASSERT_NE(childdir, nullptr) << "the salvaged dir must sit under its live parent";
    EXPECT_EQ(tree::Origin::Lost, childdir->origin());
    tree::FileNode* deep = find(childdir, "deep.txt");
    ASSERT_NE(deep, nullptr) << "the salvaged child chains under the salvaged dir";
    EXPECT_TRUE(rl->first_child() == nullptr || !find(rl, "childdir"))
        << "the reparented dir must NOT be flat in Lost Files";
    EXPECT_EQ(4u, st.journal_salvaged); /* ghost + esc (the S-004 orphan
                                         * path) + childdir + deep.txt */
}

TEST(Ext4Engine, JournalUnreferencedOrphanFlatWithRealName) {
    /* a journal dir block whose owning dir is named by NO other block: the
     * chain is unresolvable - the og's unreferenced-orphan placement
     * (flat under the lost root), and journal children keep their on-disk
     * names (the iNode%I64d fallback is the SWEEP surface's naming -
     * the og's own trace: journal children get real names) */
    auto img = build_volume();
    grow_journal(img, 8);
    u8* snap = img.block(10 + 5);
    memset(snap, 0, BS);
    u32 c = 0;
    add_dirent(snap, c, 7, ".", 2); /* owner 7 - nobody names inode 7 */
    add_dirent(snap, c, 4, "..", 2);
    add_dirent(snap, c, 9, "lonely.txt", 1);

    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, nullptr, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    bool seen = false;
    for (tree::FileNode* n = rl->first_child(); n; n = n->next_sibling()) {
        if (n->props() && n->name_utf8() == "lonely.txt") {
            seen = true;
            EXPECT_EQ(tree::Origin::Lost, n->origin());
        }
    }
    EXPECT_TRUE(seen) << "the unresolvable-chain entry surfaces flat with its real name";
}

TEST(Ext4Engine, TwoPassJournalClaimInteraction) {
    /* an entry the live tree already claimed must NOT be duplicated by the
     * journal pass (the og's reachability check FUN_180003fb0 :2807-2871:
     * the live claim wins) */
    auto img = build_volume();
    grow_journal(img, 8);
    u8* snap = img.block(10 + 5);
    memset(snap, 0, BS);
    u32 c = 0;
    add_dirent(snap, c, 4, ".", 2); /* owner: sub (LIVE) */
    add_dirent(snap, c, 2, "..", 2);
    add_dirent(snap, c, 5, "nested.bin", 1); /* INO_SUB: visited by the walk */
    add_dirent(snap, c, 3, "hello.txt", 1);  /* INO_HELLO: visited */

    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, nullptr, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    /* nested.bin appears exactly ONCE in the whole tree (under sub) */
    size_t count = 0;
    std::function<void(tree::FileNode*)> countwalk = [&](tree::FileNode* n) {
        for (tree::FileNode* x = n->first_child(); x; x = x->next_sibling()) {
            if (x->props() && x->name_utf8() == "nested.bin")
                ++count;
            countwalk(x);
        }
    };
    for (auto* r : tree.roots())
        countwalk(r);
    EXPECT_EQ(size_t(1), count) << "the live claim wins - no journal duplicate";
    EXPECT_EQ(2u, st.journal_salvaged) << "only the S-004 orphan remnants (ghost + esc) surface; "
                                          "the live-claimed entries do not duplicate";
}

/* ---------------------------------------------------------------------------
 * N-073 pin (classified FIXED-ALREADY: N-004's landing at the transport
 * layer covers the engine path N-073 filed against). The og's ext4
 * CEXT4CatchBufferDeviceProxy 1 KiB-backtrack shifted-window salvage
 * (corpus ext4-engine--deep-dive.md :25-33), proven here through the
 * ENGINE path - scan_ext4's dir-walk content reads (read_extent_data ->
 * v.dev, exactly the call chain N-073 cites), driven on the production
 * pipe shape (engine -> RetryingDevice -> fault -> image).
 *
 * The fault device models the alignment-dependent transport failure class
 * the CatchBuffer exists for: the named exact (offset,length) windows
 * fail; every other request - crucially the 1 KiB-backtrack shifted
 * windows - succeeds. The root dir content read is [B_ROOTDIR*BS,
 * +BS) = [5120, 6144); its bisect halves are (5120,512)/(5632,512);
 * the backtracks are (4096,2048) for the full window and (4608,1536)
 * for the high half. */
class ExactFaultDevice : public IStorageDevice {
public:
    ExactFaultDevice(std::unique_ptr<IStorageDevice> inner, std::set<std::pair<u64, u32>> dead)
        : inner_(std::move(inner)), dead_(std::move(dead)) {}
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (dead_.count({off, len}))
            return ERR_READ_FAILED;
        return inner_->read_at(off, buf, len);
    }
    const Geometry& geometry() const override { return inner_->geometry(); }

private:
    std::unique_ptr<IStorageDevice> inner_;
    std::set<std::pair<u64, u32>> dead_;
};

namespace {
/* N-073: the fault set defeats the same-window retries AND both bisect
 * halves; ONLY the full window's 1 KiB-backtrack (4096,2048) succeeds.
 * With backtrack ON (the og-parity default) the dir content is salvaged
 * and the tree is intact; with backtrack OFF (the pre-N-004 world N-073
 * filed: "core zero-fills the failed extent") the same read zero-fills
 * and the dir parses as nothing. (The halves' own backtracks are never
 * reached: with backtrack ON the main window fires first; with OFF no
 * backtrack runs at all.) */
const std::set<std::pair<u64, u32>> kN073DeadFull = {
    {u64(B_ROOTDIR) * BS, u32(BS)},    /* direct window (5120,1024) */
    {u64(B_ROOTDIR) * BS, 512u},       /* bisect low half (5120,512) */
    {u64(B_ROOTDIR) * BS + 512, 512u}, /* bisect high half (5632,512) */
};
} // namespace

TEST(Ext4Engine, CatchBufferBacktrackSalvage) {
    auto img = build_volume();
    auto mem = std::make_unique<MemDevice>(img.raw());
    auto dev =
        RetryingDevice::create(std::make_unique<ExactFaultDevice>(std::move(mem), kN073DeadFull),
                               RetryPolicy{3, RetryMode::Skip, /*backtrack=*/true});
    ASSERT_NE(nullptr, dev);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(dev.get(), fl, nullptr, nullptr, nullptr, tree, ra, rd, nullptr, st);
    ASSERT_EQ(OK, rc) << err_text(rc);

    std::set<std::string> found;
    std::function<void(tree::FileNode*, const std::string&)> collect = [&](tree::FileNode* n,
                                                                           const std::string& p) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (!c->props())
                continue;
            const std::string nm = c->name_utf8();
            const std::string full = p.empty() ? nm : (p == "/" ? "/" + nm : p + "/" + nm);
            found.insert(full);
            collect(c, full);
        }
    };
    collect(ra, "");
    EXPECT_TRUE(found.count("/hello.txt"));
    EXPECT_TRUE(found.count("/sub"));
    EXPECT_TRUE(found.count("/sub/nested.bin"));
    EXPECT_EQ(2u, st.files);
    /* the og-parity tail: a CatchBuffer-salvaged region is never recorded
     * bad - the shifted window succeeded, so no error-map entry exists. */
    EXPECT_EQ(0u, dev->error_map().ranges().size());
}

TEST(Ext4Engine, BacktrackOffZeroFillsTheWindow) {
    /* The control leg: identical fault, backtrack OFF = the pre-N-004 tree
     * = the exact behavior N-073 filed. The dir window zero-fills under
     * Skip mode, parse_dir_block's majority gate rejects the zeros, the
     * walk surfaces metadata-only, and the read failure is mapped. */
    auto img = build_volume();
    auto mem = std::make_unique<MemDevice>(img.raw());
    auto dev =
        RetryingDevice::create(std::make_unique<ExactFaultDevice>(std::move(mem), kN073DeadFull),
                               RetryPolicy{3, RetryMode::Skip, /*backtrack=*/false});
    ASSERT_NE(nullptr, dev);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(dev.get(), fl, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st);
    ASSERT_EQ(OK, rc) << err_text(rc);

    std::set<std::string> found;
    std::function<void(tree::FileNode*, const std::string&)> collect = [&](tree::FileNode* n,
                                                                           const std::string& p) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (!c->props())
                continue;
            const std::string nm = c->name_utf8();
            found.insert(p.empty() ? nm : (p == "/" ? "/" + nm : p + "/" + nm));
            collect(c, p.empty() ? nm : p + "/" + nm);
        }
    };
    collect(ra, "");
    EXPECT_FALSE(found.count("/hello.txt"));
    EXPECT_FALSE(found.count("/sub"));
    EXPECT_EQ(0u, st.files);
    /* zero-filled after exhaustion: both halves mapped (the pre-N-004
     * behavior - the whole span lands in the error map). */
    EXPECT_EQ(u64(1024), dev->error_map().total_bytes());
}

TEST(Ext4Engine, ZeroFillOnlyWhenBacktrackAlsoFails) {
    /* N-073's second named test. The main backtrack window (4096,2048) is
     * now also dead; bisection splits the window and only the truly
     * unreadable HALF is zero-filled: the low half (5120,512) is healthy
     * and passes at try, the high half (5632,512) is dead (its own
     * backtrack (4608,1536) is dead too). A "tail.txt" dirent placed at
     * dir offset 512 - inside the dead half - proves the split: the
     * first-half dirents surface, the dead-half one does not. */
    auto img = build_volume();
    u8* root = img.block(B_ROOTDIR);
    /* pad entry [56,512): inode 0, rec_len 456, name_len 0 - ext2's
     * reserved-entry form; keeps the parser walking across the gap so
     * the tail dirent at 512 is reachable when the half is readable. */
    put32(root + 56, 0);
    put16(root + 56 + 4, 456);
    root[56 + 6] = 0;
    root[56 + 7] = 0;
    u32 cur = 512;
    add_dirent(root, cur, INO_HELLO, "tail.txt", 1);

    auto mem = std::make_unique<MemDevice>(img.raw());
    auto dev = RetryingDevice::create(
        std::make_unique<ExactFaultDevice>(
            std::move(mem),
            std::set<std::pair<u64, u32>>{
                {u64(B_ROOTDIR) * BS, u32(BS)},            /* direct (5120,1024) */
                {u64(B_ROOTDIR) * BS - 1024, 2048u},       /* main backtrack (4096,2048) */
                {u64(B_ROOTDIR) * BS + 512, 512u},         /* high half (5632,512) */
                {u64(B_ROOTDIR) * BS - 1024 + 512, 1536u}, /* hi's backtrack (4608,1536) */
            }),
        RetryPolicy{3, RetryMode::Skip, /*backtrack=*/true});
    ASSERT_NE(nullptr, dev);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(dev.get(), fl, nullptr, nullptr, nullptr, tree, ra, rd, nullptr, st);
    ASSERT_EQ(OK, rc) << err_text(rc);

    std::set<std::string> found;
    std::function<void(tree::FileNode*, const std::string&)> collect = [&](tree::FileNode* n,
                                                                           const std::string& p) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (!c->props())
                continue;
            const std::string nm = c->name_utf8();
            const std::string full = p.empty() ? nm : (p == "/" ? "/" + nm : p + "/" + nm);
            found.insert(full);
            collect(c, full);
        }
    };
    collect(ra, "");
    /* the readable half arrived: the fixture dirents (offsets 0..56) live */
    EXPECT_TRUE(found.count("/hello.txt"));
    EXPECT_TRUE(found.count("/sub"));
    EXPECT_TRUE(found.count("/sub/nested.bin"));
    EXPECT_EQ(2u, st.files);
    /* the dead half was zero-filled, not the whole window: tail.txt (its
     * dirent sits at offset 512, inside the dead half) never surfaces. */
    EXPECT_FALSE(found.count("/tail.txt"));
    /* and only the unreadable half is in the error map. */
    ASSERT_EQ(1u, dev->error_map().ranges().size());
    EXPECT_EQ(u64(B_ROOTDIR) * BS + 512, dev->error_map().ranges()[0].offset);
    EXPECT_EQ(u64(512), dev->error_map().ranges()[0].length);
}

TEST(Ext4Engine, NanosecondsPreservedFromExtraField) {
    /* I-006 pin (added by session 14's INFO re-verification pass — the
     * finding's own "Tests to add" was never landed). The deliberate
     * superset: the core preserves i_*_extra nanoseconds (kernel
     * ext4_decode_extra_time: nsec = extra>>2) where the og forces
     * wMilliseconds = 0 (EXT4SearchDLL :5494-5515/:6262-6282 — sec-only
     * reads). Guards against anyone "fixing" parity by rounding ext4
     * precision DOWN to the og. Fixture: s_inode_size = 152 (>= 0x98
     * opens the extra-field gate), inodes packed at 152 - root and
     * hello rewritten at the new spacing. */
    auto img = build_volume();
    put16(img.block(1) + 0x58, 152); /* s_inode_size (8-aligned, in range) */
    u8* table = img.block(B_INODE_TABLE);
    memset(table, 0, BS);
    /* root (ino 2) at +152: dir, size 1 KiB, one extent -> B_ROOTDIR */
    u8* r2 = table + 152;
    put16(r2 + 0x00, 0x4000);
    put32(r2 + 0x04, BS);
    put32(r2 + 0x08, 1000);
    put32(r2 + 0x0C, 2000);
    put32(r2 + 0x10, 3000);
    put16(r2 + 0x1A, 2);
    put32(r2 + 0x20, 0x80000);
    u8* ib2 = r2 + 0x28;
    put16(ib2 + 0, 0xF30A);
    put16(ib2 + 2, 1);
    put16(ib2 + 4, 4);
    put32(ib2 + 12, 0);
    put16(ib2 + 16, 1);
    put32(ib2 + 20, u32(B_ROOTDIR));
    /* hello (ino 3) at +304: file, size 14, one extent -> B_HELLO_DATA;
     * i_mtime = 3000, i_mtime_extra = 0x5DC00000 -> nsec 393,216,000 */
    u8* r3 = table + 304;
    put16(r3 + 0x00, 0x8000);
    put32(r3 + 0x04, 14);
    put32(r3 + 0x08, 1000);
    put32(r3 + 0x0C, 2000);
    put32(r3 + 0x10, 3000);
    put16(r3 + 0x1A, 1);
    put32(r3 + 0x20, 0x80000);
    u8* ib3 = r3 + 0x28;
    put16(ib3 + 0, 0xF30A);
    put16(ib3 + 2, 1);
    put16(ib3 + 4, 4);
    put32(ib3 + 12, 0);
    put16(ib3 + 16, 1);
    put32(ib3 + 20, u32(B_HELLO_DATA));
    put16(r3 + 0x80, 0x2C);        /* i_extra_isize (covers to 0x90) */
    put32(r3 + 0x88, 0x5DC00000u); /* i_mtime_extra: nsec = 0x17700000 */

    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st);
    ASSERT_EQ(OK, rc) << err_text(rc);

    tree::FileNode* hello = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props() && c->name_utf8() == "hello.txt")
                hello = c;
            find(c);
        }
    };
    find(ra);
    ASSERT_NE(nullptr, hello) << "hello.txt must surface";
    /* base = (3000 + 11644473600) * 10^7 = 116444766000000000; plus nsec
     * 393,216,000 / 100 = 3,932,160 (100ns units). Second-only (the og's
     * precision) would be exactly the base - the nsec delta is the
     * guarded superset. */
    EXPECT_EQ(u64(116444766000000000ULL + 3932160ULL), hello->props()->times.modified)
        << "i_mtime_extra nanoseconds must survive into the 100ns FILETIME";
}

/* ---- N-016b (N-029 residual): raw name bytes carried on the props ------ */

/* ---- N-016b (N-029 residual): raw name bytes carried on the props ------ */

TEST(Ext4Engine, InvalidUtf8NameCarriesRawBytes) {
    /* The og's .rsf keeps the ORIGINAL on-disk name bytes verbatim
     * (EXT4SearchDLL imports no conversion API); the core's display name
     * is the lossless U+E000+byte mapping (M-010/D-92) - the original
     * bytes must therefore ride ALONGSIDE (props->name_raw) whenever the
     * mapping fired, so sessions persist them and the writer can derive
     * the og-parity restore filename. Fail-before: name_raw stayed empty
     * (no carrier existed). */
    auto img = build_volume();
    {
        u8* root = img.block(B_ROOTDIR);
        u32 cur = 76;
        add_dirent(root, cur, INO_HELLO, "\xff(bad", 1);
        add_dirent(root, cur, INO_SUB, "caf\xc3\xa9.txt", 1); /* valid: no raw */
    }
    MemDevice dev(img.raw());

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    ASSERT_EQ(OK, scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st))
        << err_text(0);

    tree::FileNode* bad = nullptr;
    tree::FileNode* cafe = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props() && c->props()->name == std::u16string(u"\uE0FF(bad"))
                bad = c;
            if (c->props() && c->props()->name == std::u16string(u"caf\u00e9.txt"))
                cafe = c;
            find(c);
        }
    };
    find(ra);
    ASSERT_NE(nullptr, bad) << "the invalid-UTF-8 name must be in the results";
    ASSERT_NE(nullptr, cafe);
    /* the raw bytes ride alongside the mapped display name */
    EXPECT_EQ(bad->props()->name_raw, std::string("\xff(bad"));
    /* clean UTF-8 names carry NO raw (the decoded name round-trips) */
    EXPECT_TRUE(cafe->props()->name_raw.empty());
}

/* ---- S-065: the indirect-block ladder's bounds -------------------------- */

namespace {

/* the read-counting device: proves the ladder's I/O volume (and keeps the
 * pre-fix RED runs bounded by failing reads past a cap) */
class LadderCountingDevice : public MemDevice {
public:
    explicit LadderCountingDevice(std::vector<u8> d) : MemDevice(std::move(d)) {}
    u64 reads = 0;
    u64 cap = 0; /* 0 = unlimited; otherwise the (cap+1)-th read fails */
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (cap != 0 && reads >= cap)
            return ERR_READ_FAILED;
        ++reads;
        return MemDevice::read_at(off, buf, len);
    }
};

/* a cancel token that flips to cancelled on demand */
class LadderCancelToken : public CancelToken {
public:
    bool cancelled() const override { return on_.load(); }
    void flip() { on_.store(true); }

private:
    std::atomic<bool> on_{false};
};

/* the arming device: flips the token the moment the poison block is first
 * read - the cancel responsiveness is measured from THAT point */
class LadderArmingDevice : public MemDevice {
public:
    explicit LadderArmingDevice(std::vector<u8> d) : MemDevice(std::move(d)) {}
    u64 poison_reads = 0;
    u64 poison_off = 0;
    LadderCancelToken tok;
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off == poison_off) {
            ++poison_reads;
            tok.flip();
        }
        return MemDevice::read_at(off, buf, len);
    }
};

/* plant the deleted classic-indirect inode: NO EXTENTS_FL (flags stay 0),
 * plain i_block pointers (slot 12 = single, 13 = double, 14 = triple) */
void plant_classic_gone(Img& img, u64 size, int slot, u32 pointer) {
    u8* p = img.block(B_INODE_TABLE) + size_t((INO_GONE - 1) * INO_SZ);
    memset(p, 0, INO_SZ);
    put16(p + 0x00, 0x8000);    /* regular file */
    put32(p + 0x04, u32(size)); /* i_size_lo (fixtures stay < 4 GiB) */
    put32(p + 0x14, 5000);      /* dtime: deleted */
    put16(p + 0x1A, 0);         /* links 0 */
    put32(p + 0x1C, 8);         /* i_blocks_lo (the N-177 carrier) */
    put32(p + 0x28 + u64(slot) * 4, pointer);
}

/* the self-referential poison block: every slot points back at the block
 * itself (in-range, nonzero, never converges) */
void plant_self_referential_block(Img& img, u64 block_no) {
    u8* b = img.block(block_no);
    memset(b, 0, BS);
    for (u32 i = 0; i < BS / 4; ++i)
        put32(b + u64(i) * 4, u32(block_no));
}

} // namespace

TEST(Ext4Engine, PoisonDoubleIndirectBoundedBySize) {
    /* S-065: a deleted classic inode with a 2 KiB i_size and a stale
     * DOUBLE-indirect pointer at a self-referential block. The og's ladder
     * is bounded by the file's logical size (FUN_180005490 :3952-3954 -
     * `param_4 <= lVar9` breaks the loop; size 2 KiB ends at logical block
     * 2, deep inside the 12 direct slots, so the indirect ladder is never
     * entered). Pre-fix: 1 + 256 indirection-block reads and 65536 extent
     * pushes for this one inode. The read-count comparison against the
     * identical control (no poison pointer) is the proof. */
    Img poison = build_volume();
    plant_classic_gone(poison, 2048, 13 /* i_block[13] double */, 20);
    plant_self_referential_block(poison, 20);
    Img control = build_volume();
    plant_classic_gone(control, 2048, 13, 0);

    LadderCountingDevice pd(poison.raw());
    LadderCountingDevice cd(control.raw());
    tree::ResultTree t1, t2;
    auto* ra1 = t1.add_root(u"Found");
    auto* rd1 = t1.add_root(u"Deleted");
    auto* rl1 = t1.add_root(u"Lost Files");
    auto* ra2 = t2.add_root(u"Found");
    auto* rd2 = t2.add_root(u"Deleted");
    auto* rl2 = t2.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st1, st2;
    ASSERT_EQ(OK, scan_ext4(&pd, fl, nullptr, nullptr, nullptr, t1, ra1, rd1, rl1, st1))
        << err_text(0);
    ASSERT_EQ(OK, scan_ext4(&cd, fl, nullptr, nullptr, nullptr, t2, ra2, rd2, rl2, st2))
        << err_text(0);
    EXPECT_LE(pd.reads, cd.reads) << "the size-bounded ladder reads NOTHING the control does not "
                                     "(the whole indirect subtree is past EOF at size 2 KiB)";
}

TEST(Ext4Engine, PoisonTripleIndirectBoundedBySize) {
    /* S-065's headline case: the stale TRIPLE-indirect pointer. Pre-fix the
     * self-referential block drives 1 + 256*(1+256) ~ 65k reads and 256^3
     * ~ 16.7M extent pushes (freeze/OOM); the counting device's read cap
     * keeps the RED run bounded while pinning that the walk BLEW PAST the
     * size bound. Post-fix the triple subtree (logical start
     * 12 + 256 + 65536) is far past the 2 KiB size - zero extra reads. */
    Img poison = build_volume();
    plant_classic_gone(poison, 2048, 14 /* i_block[14] triple */, 20);
    plant_self_referential_block(poison, 20);
    Img control = build_volume();
    plant_classic_gone(control, 2048, 14, 0);

    LadderCountingDevice pd(poison.raw());
    pd.cap = 3000; /* bounded RED: the pre-fix walk wants ~65k reads */
    LadderCountingDevice cd(control.raw());
    tree::ResultTree t1, t2;
    auto* ra1 = t1.add_root(u"Found");
    auto* rd1 = t1.add_root(u"Deleted");
    auto* rl1 = t1.add_root(u"Lost Files");
    auto* ra2 = t2.add_root(u"Found");
    auto* rd2 = t2.add_root(u"Deleted");
    auto* rl2 = t2.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st1, st2;
    (void)scan_ext4(&pd, fl, nullptr, nullptr, nullptr, t1, ra1, rd1, rl1, st1);
    ASSERT_EQ(OK, scan_ext4(&cd, fl, nullptr, nullptr, nullptr, t2, ra2, rd2, rl2, st2))
        << err_text(0);
    EXPECT_LE(pd.reads, cd.reads) << "the triple-indirect subtree past EOF is never entered";
}

TEST(Ext4Engine, StaleIndirectCancelResponsive) {
    /* S-065: the ladder consults the cancel token inside the walk. The
     * arming device flips the token when the poison block is FIRST read;
     * the fixed ladder's next recursion (the cancel check runs at every
     * indirection-block boundary) stops immediately - the poison block is
     * read at most once. Pre-fix: no check inside the walk, 257 reads of
     * the poison block. */
    Img img = build_volume();
    plant_classic_gone(img, 1ull << 30 /* 1 GiB: the ladder IS in range */, 13, 20);
    plant_self_referential_block(img, 20);

    LadderArmingDevice dev(img.raw());
    dev.poison_off = 20 * BS;
    tree::ResultTree t;
    auto* ra = t.add_root(u"Found");
    auto* rd = t.add_root(u"Deleted");
    auto* rl = t.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    (void)scan_ext4(&dev, fl, &dev.tok, nullptr, nullptr, t, ra, rd, rl, st);
    EXPECT_LE(dev.poison_reads, 1u)
        << "cancel honored inside the walk: the poison block is read at most "
           "once before the walk stops";
}

TEST(Ext4Engine, ContiguousPhysicalBlocksMergedToSingleExtent) {
    /* S-065: the og's observable output shape - physically-contiguous
     * blocks merge into ONE extent as the walk proceeds (FUN_180005490
     * :3986-3996, the merge branch that bounds the vector by physical-run
     * count). A 16 KiB classic file whose single-indirect block points at
     * four consecutive blocks emits ONE 4-block extent, not four. */
    Img img = build_volume();
    plant_classic_gone(img, 16 * BS, 12 /* i_block[12] single */, 20);
    u8* ib = img.block(20);
    memset(ib, 0, BS);
    for (u32 i = 0; i < 4; ++i)
        put32(ib + u64(i) * 4, 21 + i); /* consecutive physical blocks */

    MemDevice dev(img.raw());
    tree::ResultTree t;
    auto* ra = t.add_root(u"Found");
    auto* rd = t.add_root(u"Deleted");
    auto* rl = t.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    ASSERT_EQ(OK, scan_ext4(&dev, fl, nullptr, nullptr, nullptr, t, ra, rd, rl, st)) << err_text(0);

    const tree::FileNode* gone = nullptr;
    for (tree::FileNode* c = rd->first_child(); c; c = c->next_sibling()) {
        if (c->props() && c->name_utf8() == "Inode6")
            gone = c;
    }
    ASSERT_NE(nullptr, gone) << "the deleted classic inode must surface";
    ASSERT_FALSE(gone->props()->extents.empty()) << "its extents must be emitted";
    ASSERT_EQ(u64(1), gone->props()->extents.size())
        << "four physically-consecutive blocks = ONE merged extent";
    EXPECT_EQ(u64(21) * BS, gone->props()->extents.front().offset);
    EXPECT_EQ(u64(4) * BS, gone->props()->extents.front().length);
}

/* ---- M-152: the orphan-active population surfaces (the og's
 * predicate-free sweep filter, raw-asm verified at 0x180005f5d-0x180005f87:
 * size_lo, EXTENTS_FL, the 0xF30A magic, eh_entries - NO dtime, NO links) */

TEST(Ext4Engine, OrphanActiveInodeSurfacesInSweep) {
    /* the finding's falsification fixture: a regular file with
     * extent-mapped data whose parent directory's entries are damaged -
     * dtime==0, links==1, size>0, EXTENTS_FL, a valid extent header, and
     * NO dirent anywhere pointing at it. The og's filter admits it by
     * construction; the core's old `dtime==0 && links!=0` skip dropped
     * it before any content gate ran. RED at the pre-fix tree (the file
     * appears in NO branch), GREEN here: the file surfaces under the
     * deleted root with its extents and the honest Active state (no
     * deletion evidence on the inode). */
    auto img = build_volume();
    /* inode 7: the orphan-active (table slots 1..8; 7 is unwritten in
     * the base fixture) - data on a free block (the bitmap frees 15+) */
    constexpr u64 B_ORPHAN_DATA = 20;
    write_inode(img, 7, 0x8000, 24, B_ORPHAN_DATA, 1, 1, 0);
    memcpy(img.block(B_ORPHAN_DATA), "orphaned-but-intact!", 20);

    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(1u, st.orphan_inodes) << "the orphan-active population is counted";

    tree::FileNode* orphan = nullptr;
    for (tree::FileNode* c = rd->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->record_ref == 7)
            orphan = c;
    ASSERT_NE(nullptr, orphan) << "the orphan-active inode MUST surface (M-152)";
    EXPECT_EQ(tree::DeletedState::Active, orphan->deleted_state())
        << "no deletion evidence: the state stays honest to the inode";
    EXPECT_FALSE(orphan->props()->extents.empty());
    EXPECT_EQ(u64(B_ORPHAN_DATA * BS), orphan->props()->extents[0].offset);
    EXPECT_EQ(u64(BS), orphan->props()->extents[0].length);
    EXPECT_EQ(u64(24), orphan->props()->logical_size);
}

TEST(Ext4Engine, OrphanSweepDedupsAgainstLiveTree) {
    /* the live claim wins: hello.txt (dtime==0, links==1) IS reachable
     * from the root directory - the visited_inodes guard keeps the sweep
     * from double-emitting it as an orphan. The deleted root carries the
     * dtime-set population only (plus no orphan copies of live files). */
    auto img = build_volume();
    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(0u, st.orphan_inodes)
        << "every live-shaped inode in the base fixture is either claimed "
           "(hello/sub) or the journal inode (the machinery's claim)";
    for (tree::FileNode* c = rd->first_child(); c; c = c->next_sibling())
        ASSERT_NE(u64(3), c->props()->record_ref)
            << "hello.txt must NOT double-emit as an orphan copy";
}

TEST(Ext4Engine, SweepModeGateHolds) {
    /* the registered hardening pin: the og's filter is shape-only and
     * would admit a FIFO/socket with a forged extent header; the core
     * keeps the regular/dir mode gate (PARITY-DELTAS carries the row). */
    auto img = build_volume();
    write_inode(img, 7, 0x1000 /* S_IFIFO */, 24, 20, 1, 1, 0);
    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(0u, st.orphan_inodes);
    for (tree::FileNode* c = rd->first_child(); c; c = c->next_sibling())
        EXPECT_NE(u64(7), c->props()->record_ref)
            << "the forged-extent FIFO stays excluded (the mode gate)";
}

TEST(Ext4Engine, SweepStatsCountOrphanAdmissions) {
    /* the two populations are countable separately: the base fixture's
     * dtime-set inode (ino 6) counts deleted_inodes; an added
     * orphan-active counts orphan_inodes; the journal inode (ino 8,
     * dtime==0 links==1) counts NEITHER (the machinery's claim). */
    auto img = build_volume();
    write_inode(img, 7, 0x8000, 24, 20, 1, 1, 0);
    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(1u, st.deleted_inodes) << "the dtime-set population (unchanged)";
    EXPECT_EQ(1u, st.orphan_inodes) << "the new admission (ino 7)";
}

} // namespace ext4
} // namespace cr
