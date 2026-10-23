/* test_ext4_volume.cpp - synthetic ext4 volume end-to-end.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/ext4/cr_ext4_engine.h"
#include <gtest/gtest.h>
#include <cstring>
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

class MemDevice final : public IStorageDevice {
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

} // namespace
} // namespace ext4
} // namespace cr
