/* test_ext4_volume.cpp - synthetic ext4 volume end-to-end.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/ext4/cr_ext4_engine.h"
#include "cr/storage/cr_retry.h"
#include <utility>
#include <gtest/gtest.h>
#include <cstring>
#include <functional>
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
    EXPECT_EQ(by_utf8["\xee\x83\xbf(bad"],
              std::u16string(u"\uE0FF(bad"));
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
    ExactFaultDevice(std::unique_ptr<IStorageDevice> inner,
                     std::set<std::pair<u64, u32>> dead)
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
    {u64(B_ROOTDIR) * BS, u32(BS)},        /* direct window (5120,1024) */
    {u64(B_ROOTDIR) * BS, 512u},           /* bisect low half (5120,512) */
    {u64(B_ROOTDIR) * BS + 512, 512u},     /* bisect high half (5632,512) */
};
} // namespace

TEST(Ext4Engine, CatchBufferBacktrackSalvage) {
    auto img = build_volume();
    auto mem = std::make_unique<MemDevice>(img.raw());
    auto dev = RetryingDevice::create(
        std::make_unique<ExactFaultDevice>(std::move(mem), kN073DeadFull),
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
    std::function<void(tree::FileNode*, const std::string&)> collect =
        [&](tree::FileNode* n, const std::string& p) {
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
    auto dev = RetryingDevice::create(
        std::make_unique<ExactFaultDevice>(std::move(mem), kN073DeadFull),
        RetryPolicy{3, RetryMode::Skip, /*backtrack=*/false});
    ASSERT_NE(nullptr, dev);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    u32 rc = scan_ext4(dev.get(), fl, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st);
    ASSERT_EQ(OK, rc) << err_text(rc);

    std::set<std::string> found;
    std::function<void(tree::FileNode*, const std::string&)> collect =
        [&](tree::FileNode* n, const std::string& p) {
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
                {u64(B_ROOTDIR) * BS, u32(BS)},         /* direct (5120,1024) */
                {u64(B_ROOTDIR) * BS - 1024, 2048u},    /* main backtrack (4096,2048) */
                {u64(B_ROOTDIR) * BS + 512, 512u},      /* high half (5632,512) */
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
    std::function<void(tree::FileNode*, const std::string&)> collect =
        [&](tree::FileNode* n, const std::string& p) {
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

} // namespace
} // namespace ext4
} // namespace cr

} // namespace
