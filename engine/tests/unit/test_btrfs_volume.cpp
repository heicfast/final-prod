/* test_btrfs_volume.cpp - synthetic Btrfs volume end-to-end.
 *
 * Identity logical->physical mapping keeps the fixture readable: logical X
 * is always at physical X.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/btrfs/cr_btrfs_engine.h"
#include "cr/cr_text.h"
#include <gtest/gtest.h>
#include <zlib.h>
#include "cr/compress/cr_decomp.h"
#include <cstring>
#include <map>
#include <set>

namespace cr {
namespace btrfs {
namespace {

constexpr u32 NODESIZE = 4096;
constexpr u64 IMG_BYTES = 8ull << 20;

/* logical == physical everywhere (identity chunk) */
constexpr u64 L_CHUNK_TREE = 0x100000;
constexpr u64 L_ROOT_TREE = 0x102000;
constexpr u64 L_FS_TREE = 0x104000;
constexpr u64 L_DATA = 0x700000; /* inside the data chunk */

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
void put64(u8* p, u64 v) {
    put32(p, u32(v));
    put32(p + 4, u32(v >> 32));
}

/* local key type (engine's Key is internal) */
struct Key {
    u64 objectid = 0;
    u8 type = 0;
    u64 offset = 0;
};

/* distinctive fsid/chunk_tree_uuid so the node-fsid and chunk-tree-uuid
 * gates in read_node are exercised (zeros would make the memcmp trivially
 * pass and the check untested) */
constexpr u8 FSID[16] = {0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
                         0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF};
constexpr u8 CTUUID[16] = {0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
                           0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF};

/* item buffer being assembled for one node */
struct NodeBuilder {
    std::vector<u8> node;
    struct Item {
        Key key;
        std::vector<u8> data;
    };
    std::vector<Item> items;

    NodeBuilder() { node.assign(NODESIZE, 0); }

    /* kernel btrfs_header: bytenr@0x30, generation@0x50, owner@0x58,
     * nritems@0x60, level@0x64; items array @0x65. fsid@0x20 (always the
     * SB fsid) and chunk_tree_uuid@0x40 (chunk-tree nodes only). */
    void header(u64 bytenr, u32 nritems, u8 level) {
        put64(node.data() + 0x30, bytenr); /* self-referential */
        put64(node.data() + 0x50, 1);      /* generation */
        put64(node.data() + 0x58, 5);      /* owner (tree id) */
        put32(node.data() + 0x60, nritems);
        node[0x64] = level;
        memcpy(node.data() + 0x20, FSID, 16);
        memcpy(node.data() + 0x40, CTUUID, 16);
    }

    void add(const Key& k, const std::vector<u8>& data) { items.push_back({k, data}); }

    /* finalize: pack data from the node end, then write the item array at
     * 0x65. item.offset counts from the items-array start (kernel rule:
     * data_ptr = offsetof(items) + offset). */
    std::vector<u8> finish(u64 bytenr, u8 level) {
        header(bytenr, u32(items.size()), level);
        u32 end = NODESIZE;
        std::vector<u32> data_offsets;
        for (const auto& it : items) {
            end -= u32(it.data.size());
            memcpy(node.data() + end, it.data.data(), it.data.size());
            data_offsets.push_back(end);
        }
        constexpr u32 ITEMS_AT = 0x65;
        for (u32 i = 0; i < items.size(); ++i) {
            u8* it = node.data() + ITEMS_AT + u64(i) * 25;
            put64(it + 0, items[i].key.objectid);
            it[8] = items[i].key.type;
            put64(it + 9, items[i].key.offset);
            put32(it + 17, data_offsets[i] - ITEMS_AT); /* from items start */
            put32(it + 21, u32(items[i].data.size()));
        }
        return node;
    }

    /* internal node: btrfs_key_ptr array { key(17), blockptr u64@17,
     * generation u64@25 } at 0x65 (kernel format). */
    std::vector<u8> finish_internal(u64 bytenr, u8 level,
                                    const std::vector<std::pair<Key, u64>>& ptrs) {
        header(bytenr, u32(ptrs.size()), level);
        constexpr u32 ITEMS_AT = 0x65;
        for (u32 i = 0; i < ptrs.size(); ++i) {
            u8* kp = node.data() + ITEMS_AT + u64(i) * 33;
            put64(kp + 0, ptrs[i].first.objectid);
            kp[8] = ptrs[i].first.type;
            put64(kp + 9, ptrs[i].first.offset);
            put64(kp + 17, ptrs[i].second); /* blockptr */
            put64(kp + 25, 1);              /* generation */
        }
        return node;
    }
};

std::vector<u8> make_root_item(u64 tree_bytenr, u64 root_dirid) {
    std::vector<u8> r(0xEF, 0);
    put64(r.data() + 0xA8, root_dirid);
    put64(r.data() + 0xB0, tree_bytenr);
    r[0xEE] = 0; /* level */
    return r;
}

std::vector<u8> make_inode(u32 mode, u64 size, u32 nlink, u64 mtime_sec) {
    std::vector<u8> r(160, 0);
    put64(r.data() + 0x10, size);
    put32(r.data() + 0x28, nlink);
    put32(r.data() + 0x34, mode);
    /* kernel btrfs_inode_item: reserved[4] occupies 0x50..0x70; the
     * 12-byte timespecs live at atime@0x70, ctime@0x7C, mtime@0x88,
     * otime@0x94 (each: sec i64 + nsec u32) */
    put64(r.data() + 0x70, mtime_sec); /* atime sec */
    put32(r.data() + 0x70 + 8, 0);     /* atime nsec */
    put64(r.data() + 0x7C, mtime_sec); /* ctime */
    put64(r.data() + 0x88, mtime_sec); /* mtime */
    put64(r.data() + 0x94, mtime_sec); /* otime */
    return r;
}

std::vector<u8> make_dir_item(u64 child, const char* name, u8 type) {
    /* kernel btrfs_dir_item: location(17) + transid u64@17 + data_len
     * u16@25 + name_len u16@27 + type u8@29 + name[]@30 */
    const u32 nlen = u32(strlen(name));
    std::vector<u8> r(30 + nlen, 0);
    put64(r.data() + 0, child);
    r[8] = type;             /* location.type: 1 file, 2 dir */
    put64(r.data() + 17, 1); /* transid */
    put16(r.data() + 25, 0); /* data_len */
    put16(r.data() + 27, u16(nlen));
    r[29] = type; /* BTRFS_FT_* of the entry itself */
    memcpy(r.data() + 30, name, nlen);
    return r;
}

std::vector<u8> make_extent_data(u64 disk_bytenr, u64 num_bytes) {
    std::vector<u8> r(53, 0);
    put64(r.data() + 0, 1);         /* generation */
    put64(r.data() + 8, num_bytes); /* ram_bytes */
    r[20] = 1;                      /* type: regular */
    put64(r.data() + 21, disk_bytenr);
    put64(r.data() + 29, num_bytes);
    put64(r.data() + 37, 0); /* offset in */
    put64(r.data() + 45, num_bytes);
    return r;
}

std::vector<u8> build_volume() {
    std::vector<u8> d(size_t(IMG_BYTES), 0);

    /* --- superblock at 0x10000 --- */
    u8* sb = d.data() + 0x10000;
    put64(sb + 0x40, 0x4D5F53665248425FULL); /* magic */
    memcpy(sb + 0x20, FSID, 16);             /* fsid */
    memcpy(sb + 0x60, CTUUID, 16);           /* chunk_tree_uuid */
    put64(sb + 0x48, 42);                    /* generation */
    put32(sb + 0x90, 4096);                  /* sectorsize */
    put32(sb + 0x94, 4096);                  /* nodesize */
    /* sys chunk array @0x32B: one identity chunk, logical 0x100000, 6 MiB,
     * type SYSTEM|METADATA|DATA(=7), 1 stripe at physical 0x100000 */
    {
        u32 pos = 0x32B;
        /* key: objectid 256? (chunk keys: objectid = first objectid...),
         * type CHUNK(228), offset = logical start 0x100000 */
        put64(sb + pos, 256);
        sb[pos + 8] = 228;
        put64(sb + pos + 9, 0x100000);
        pos += 17;
        /* chunk: 7 MiB covering [0x100000, 0x800000) */
        put64(sb + pos + 0, 7ull << 20);
        put64(sb + pos + 8, 3);             /* owner */
        put64(sb + pos + 0x10, 4096);       /* stripe_len */
        put64(sb + pos + 0x18, 7);          /* type: DATA|SYSTEM|METADATA */
        put32(sb + pos + 0x20, 4096);       /* io_align */
        put32(sb + pos + 0x24, 4096);       /* io_width */
        put32(sb + pos + 0x28, 4096);       /* sector_size */
        put16(sb + pos + 0x2C, 1);          /* num_stripes */
        put16(sb + pos + 0x2E, 0);          /* sub_stripes */
        put64(sb + pos + 48, 1);            /* stripe0 devid */
        put64(sb + pos + 48 + 8, 0x100000); /* stripe0 physical offset */
        const u32 total = 17 + 48 + 32;
        put32(sb + 0xA0, total); /* sys_chunk_array_size */
    }

    /* --- chunk tree (leaf at L_CHUNK_TREE): one CHUNK item (the identity
     * chunk again, so the full map covers the data region) --- */
    {
        NodeBuilder nb;
        Key ck;
        ck.objectid = 256;
        ck.type = 228;
        ck.offset = 0x100000;
        std::vector<u8> chunk(48 + 32, 0);
        put64(chunk.data() + 0, 7ull << 20);
        put64(chunk.data() + 8, 3);
        put64(chunk.data() + 0x10, 4096);
        put64(chunk.data() + 0x18, 7);
        put32(chunk.data() + 0x20, 4096);
        put32(chunk.data() + 0x24, 4096);
        put32(chunk.data() + 0x28, 4096);
        put16(chunk.data() + 0x2C, 1);
        put64(chunk.data() + 48, 1);
        put64(chunk.data() + 48 + 8, 0x100000);
        nb.add(ck, chunk);
        auto nd = nb.finish(L_CHUNK_TREE, 0);
        memcpy(d.data() + L_CHUNK_TREE, nd.data(), NODESIZE);
    }

    /* --- root tree (leaf at L_ROOT_TREE): ROOT_ITEM for FS_TREE(5) and
     * CHUNK_TREE(3) (the CHUNK_TREE root item is the discovery anchor) --- */
    {
        NodeBuilder nb;
        Key k5;
        k5.objectid = 5;
        k5.type = 132;
        k5.offset = 0;
        nb.add(k5, make_root_item(L_FS_TREE, 256));
        Key k3;
        k3.objectid = 3;
        k3.type = 132;
        k3.offset = 0;
        nb.add(k3, make_root_item(L_CHUNK_TREE, 0));
        auto nd = nb.finish(L_ROOT_TREE, 0);
        memcpy(d.data() + L_ROOT_TREE, nd.data(), NODESIZE);
    }

    /* --- FS tree (leaf at L_FS_TREE) --- */
    {
        NodeBuilder nb;
        const u64 MTIME = 1600000000;
        /* inodes: 256 root dir, 257 hello.txt, 258 sub dir, 259 nested.bin */
        Key ki;
        ki.type = 1; /* INODE_ITEM */
        ki.objectid = 256;
        nb.add(ki, make_inode(0x4000, NODESIZE, 2, MTIME));
        ki.objectid = 257;
        nb.add(ki, make_inode(0x8000, 14, 1, MTIME));
        ki.objectid = 258;
        nb.add(ki, make_inode(0x4000, NODESIZE, 2, MTIME));
        ki.objectid = 259;
        nb.add(ki, make_inode(0x8000, 22, 1, MTIME));
        /* extent data for hello.txt + nested.bin (logical L_DATA mapped by
         * the identity chunk) */
        Key ke;
        ke.type = 108; /* EXTENT_DATA */
        ke.objectid = 257;
        ke.offset = 0;
        nb.add(ke, make_extent_data(L_DATA, 4096));
        ke.objectid = 259;
        nb.add(ke, make_extent_data(L_DATA + 4096, 4096));
        /* dir entries */
        Key kd;
        kd.type = 84; /* DIR_ITEM */
        kd.objectid = 256;
        kd.offset = 0x11110001;
        nb.add(kd, make_dir_item(257, "hello.txt", 1));
        kd.offset = 0x22220002;
        nb.add(kd, make_dir_item(258, "sub", 2));
        kd.objectid = 258;
        kd.offset = 0x33330003;
        nb.add(kd, make_dir_item(259, "nested.bin", 1));
        auto nd = nb.finish(L_FS_TREE, 0);
        memcpy(d.data() + L_FS_TREE, nd.data(), NODESIZE);
    }

    /* file content */
    memcpy(d.data() + L_DATA, "Hello, btrfs!", 14);
    memcpy(d.data() + L_DATA + 4096, "nested btrfs content", 21);
    return d;
}

TEST(Btrfs, SuperblockValidation) {
    std::vector<u8> d(size_t(IMG_BYTES), 0);
    u8* sb = d.data() + 0x10000;
    put64(sb + 0x40, 0x4D5F53665248425FULL);
    memcpy(sb + 0x20, FSID, 16);
    memcpy(sb + 0x60, CTUUID, 16);
    put64(sb + 0x48, 7);
    put32(sb + 0x90, 4096);
    put32(sb + 0x94, 4096);
    SuperBlock out;
    EXPECT_TRUE(validate_superblock(sb, out));
    EXPECT_EQ(4096u, out.sectorsize);
    EXPECT_EQ(4096u, out.nodesize);
    EXPECT_EQ(u64(7), out.generation);

    /* bad magic */
    put64(sb + 0x40, 0x1122334455667788ULL);
    EXPECT_FALSE(validate_superblock(sb, out));
    put64(sb + 0x40, 0x4D5F53665248425FULL);

    /* non-pow2 sectorsize */
    put32(sb + 0x90, 3000);
    EXPECT_FALSE(validate_superblock(sb, out));
    put32(sb + 0x90, 4096);

    /* nodesize band */
    put32(sb + 0x94, 64);
    EXPECT_FALSE(validate_superblock(sb, out));
    put32(sb + 0x94, 65536 + 1);
    EXPECT_FALSE(validate_superblock(sb, out));
    put32(sb + 0x94, 4096);

    /* nodesize % sectorsize */
    put32(sb + 0x94, 6144);
    EXPECT_FALSE(validate_superblock(sb, out));
}

TEST(Btrfs, NonPow2NodesizeRejectedRegistered) {
    /* N-006 pin: 24 KiB nodesize is INSIDE the og's band [0x86, 0x10000)
     * and 0x6000 % 0x1000 == 0, so the og accepts it (no pow2 requirement,
     * BtrfsSearchDLL :10302-10304) - real mkfs.btrfs only ever writes
     * pow2 nodesizes, so this is corrupt-SB territory. The core's pow2
     * rejection is the REGISTERED deliberate hardening (D-61): a forged
     * non-pow2 nodesize would size node-buffer allocations from an
     * attacker-influenced field. Refusal routes the volume to other
     * engines/carving - a strictly better outcome than the og's
     * garbage attempt. Do not "fix" parity by loosening this. */
    std::vector<u8> img = build_volume();
    u8* sb = img.data() + 0x10000;
    put32(sb + 0x94, 0x6000); /* 24 KiB: og-band-legal, 0x6000%0x1000==0 */
    SuperBlock out{};
    EXPECT_FALSE(validate_superblock(sb, out));
    put32(sb + 0x94, 0x8000); /* 32 KiB: pow2, in-band - accepted */
    EXPECT_TRUE(validate_superblock(sb, out));
}

TEST(Btrfs, EndToEndWalk) {
    auto img = build_volume();
    MemDevice dev(img);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    BtrfsScanStats st;
    u32 rc = scan_btrfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(1u, st.subvolumes);
    EXPECT_EQ(2u, st.files);
    EXPECT_EQ(2u, st.dirs);   /* subvol root dir + sub */
    EXPECT_EQ(2u, st.chunks); /* sys chunk + chunk-tree chunk (same mapping) */

    /* collect paths */
    std::set<std::string> paths;
    std::function<void(tree::FileNode*, std::string)> walk = [&](tree::FileNode* n,
                                                                 const std::string& p) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (!c->props())
                continue;
            std::string nm = c->name_utf8();
            std::string full = p.empty() ? nm : p + "/" + nm;
            paths.insert(full);
            walk(c, full);
        }
    };
    walk(ra, "");

    EXPECT_TRUE(paths.count("subvol-5"));
    EXPECT_TRUE(paths.count("subvol-5/hello.txt"));
    EXPECT_TRUE(paths.count("subvol-5/sub"));
    EXPECT_TRUE(paths.count("subvol-5/sub/nested.bin"));

    /* verify extents + sizes */
    int checked = 0;
    for (tree::FileNode* sv = ra->first_child(); sv; sv = sv->next_sibling())
        for (tree::FileNode* c = sv->first_child(); c; c = c->next_sibling()) {
            if (!c->props())
                continue;
            if (c->name_utf8() == "hello.txt") {
                EXPECT_EQ(u64(14), c->props()->logical_size);
                ASSERT_EQ(1u, c->props()->extents.size());
                EXPECT_EQ(L_DATA, c->props()->extents[0].offset);
                EXPECT_EQ(4096u, c->props()->extents[0].length);
                ++checked;
            }
            if (c->name_utf8() == "sub")
                for (tree::FileNode* n2 = c->first_child(); n2; n2 = n2->next_sibling())
                    if (n2->props() && n2->name_utf8() == "nested.bin") {
                        EXPECT_EQ(u64(22), n2->props()->logical_size);
                        EXPECT_EQ(L_DATA + 4096, n2->props()->extents[0].offset);
                        ++checked;
                    }
        }
    EXPECT_EQ(2, checked);
}

TEST(Btrfs, BadMagicRejected) {
    std::vector<u8> d(1ull << 20, 0);
    MemDevice dev(d);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"F");
    BtrfsScanStats st;
    EXPECT_EQ(ERR_BAD_MAGIC,
              scan_btrfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));
}

TEST(Btrfs, PackedDirItemsInOneItem) {
    /* kernel packs several btrfs_dir_item records into a single DIR_ITEM
     * item; the walk must iterate all of them (audit fix - the old code
     * read only the first). */
    auto img = build_volume();
    /* rebuild the FS-tree node: two entries under ONE item */
    {
        NodeBuilder nb;
        Key ki;
        ki.type = 1;       /* INODE_ITEM */
        ki.objectid = 256; /* subvol root dir */
        nb.add(ki, make_inode(0x4000, NODESIZE, 2, 1700000000ull));
        ki.objectid = 257;
        nb.add(ki, make_inode(0x8000, 14, 1, 1700000000ull));
        ki.objectid = 258;
        nb.add(ki, make_inode(0x4000, NODESIZE, 2, 1700000000ull));
        Key ke;
        ke.type = 108;
        ke.objectid = 257;
        ke.offset = 0;
        nb.add(ke, make_extent_data(L_DATA, 4096));
        /* ONE DIR_ITEM item containing BOTH entries back-to-back */
        std::vector<u8> packed = make_dir_item(257, "hello.txt", 1);
        {
            std::vector<u8> second = make_dir_item(258, "sub", 2);
            packed.insert(packed.end(), second.begin(), second.end());
        }
        Key kd;
        kd.type = 84;
        kd.objectid = 256;
        kd.offset = 0x11110001;
        nb.add(kd, packed);
        auto nd = nb.finish(L_FS_TREE, 0);
        memcpy(img.data() + L_FS_TREE, nd.data(), NODESIZE);
    }
    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"F");
    BtrfsScanStats st;
    ASSERT_EQ(OK, scan_btrfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));
    std::set<std::string> names;
    for (tree::FileNode* c = ra->first_child(); c; c = c->next_sibling())
        for (tree::FileNode* x = c->first_child(); x; x = x->next_sibling())
            if (x->props())
                names.insert(x->name_utf8());
    EXPECT_TRUE(names.count("hello.txt"));
    EXPECT_TRUE(names.count("sub"));
}

TEST(Btrfs, MirrorSelectionNewestGeneration) {
    /* corrupt the primary superblock; the mirror set {64K, 64M, 256G, 1P}
     * (kernel btrfs_sb_offset + og .rdata table) must recover the volume
     * from the 64 MiB mirror when the image is large enough to hold it. */
    constexpr u64 IMG2 = 96ull << 20; /* > 64 MiB mirror slot */
    auto img = build_volume();
    std::vector<u8> big(size_t(IMG2), 0);
    memcpy(big.data(), img.data(), img.size());
    /* smash the primary superblock */
    memset(big.data() + 0x10000, 0xAB, 4096);
    /* place a copy of the (valid) superblock at the 64 MiB mirror */
    memcpy(big.data() + 0x4000000, img.data() + 0x10000, 4096);
    /* raise the mirror's generation so it wins even if both were valid */
    put64(big.data() + 0x4000000 + 0x48, 99);

    MemDevice dev(big);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"F");
    BtrfsScanStats st;
    ASSERT_EQ(OK, scan_btrfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));
    EXPECT_EQ(1u, st.subvolumes);
    /* and the walked file proves the chunk map still resolved */
    bool found = false;
    for (tree::FileNode* sv = ra->first_child(); sv; sv = sv->next_sibling())
        for (tree::FileNode* c = sv->first_child(); c; c = c->next_sibling())
            if (c->props() && c->name_utf8() == "hello.txt")
                found = true;
    EXPECT_TRUE(found);
}

TEST(Btrfs, InternalKeyPtrWalk) {
    /* multi-level FS tree: root tree -> FS-tree internal node -> two
     * leaves, reached through the kernel btrfs_key_ptr array (the old
     * code read internal nodes as if they were item arrays - never
     * exercised by this fixture before the audit). */
    auto img = build_volume();

    /* leaf A: inode 256 + dir entries for hello.txt */
    u8* leaf_a = img.data() + L_FS_TREE;
    {
        NodeBuilder nb;
        Key ki;
        ki.type = 1;
        ki.objectid = 256;
        nb.add(ki, make_inode(0x4000, NODESIZE, 2, 1600000000ull));
        ki.objectid = 257;
        nb.add(ki, make_inode(0x8000, 14, 1, 1600000000ull));
        Key ke;
        ke.type = 108;
        ke.objectid = 257;
        ke.offset = 0;
        nb.add(ke, make_extent_data(L_DATA, 4096));
        Key kd;
        kd.type = 84;
        kd.objectid = 256;
        kd.offset = 0x11110001;
        nb.add(kd, make_dir_item(257, "hello.txt", 1));
        auto nd = nb.finish(L_FS_TREE, 0);
        memcpy(leaf_a, nd.data(), NODESIZE);
    }
    /* leaf B: inode 258/259 + sub/nested entries */
    const u64 L_LEAF_B = L_FS_TREE + NODESIZE;
    {
        NodeBuilder nb;
        Key ki;
        ki.type = 1;
        ki.objectid = 258;
        nb.add(ki, make_inode(0x4000, NODESIZE, 2, 1600000000ull));
        ki.objectid = 259;
        nb.add(ki, make_inode(0x8000, 22, 1, 1600000000ull));
        Key ke;
        ke.type = 108;
        ke.objectid = 259;
        ke.offset = 0;
        nb.add(ke, make_extent_data(L_DATA + 4096, 4096));
        Key kd;
        kd.type = 84;
        kd.objectid = 256;
        kd.offset = 0x22220002;
        nb.add(kd, make_dir_item(258, "sub", 2));
        kd.objectid = 258;
        kd.offset = 0x33330003;
        nb.add(kd, make_dir_item(259, "nested.bin", 1));
        auto nd = nb.finish(L_LEAF_B, 0);
        memcpy(img.data() + L_LEAF_B, nd.data(), NODESIZE);
    }
    /* internal FS-tree node pointing at both leaves */
    const u64 L_INTERNAL = L_FS_TREE + 2 * NODESIZE;
    {
        NodeBuilder nb;
        Key lo; /* low key: (256, ...) */
        lo.objectid = 256;
        lo.type = 1;
        lo.offset = 0;
        Key hi; /* high key */
        hi.objectid = 258;
        hi.type = 84;
        hi.offset = 0x33330003;
        auto nd = nb.finish_internal(L_INTERNAL, 1, {{lo, L_FS_TREE}, {hi, L_LEAF_B}});
        memcpy(img.data() + L_INTERNAL, nd.data(), NODESIZE);
    }
    /* repoint the root item at the internal node */
    {
        NodeBuilder nb;
        Key k5;
        k5.objectid = 5;
        k5.type = 132;
        k5.offset = 0;
        nb.add(k5, make_root_item(L_INTERNAL, 256));
        Key k3;
        k3.objectid = 3;
        k3.type = 132;
        k3.offset = 0;
        nb.add(k3, make_root_item(L_CHUNK_TREE, 0));
        auto nd = nb.finish(L_ROOT_TREE, 0);
        memcpy(img.data() + L_ROOT_TREE, nd.data(), NODESIZE);
    }

    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"F");
    BtrfsScanStats st;
    ASSERT_EQ(OK, scan_btrfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));
    EXPECT_EQ(1u, st.subvolumes);
    EXPECT_EQ(2u, st.files);
    EXPECT_EQ(0u, st.damaged_nodes);
    std::set<std::string> names;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props())
                names.insert(c->name_utf8());
            collect(c);
        }
    };
    collect(ra);
    EXPECT_TRUE(names.count("hello.txt"));
    EXPECT_TRUE(names.count("sub"));
    EXPECT_TRUE(names.count("nested.bin"));
}

TEST(Btrfs, InlineAndCompressedExtents) {
    /* BTR-3 (AUDIT-LINUX-FS, F-A2-13): inline EXTENT_DATA content is the
     * item payload after the 21-byte header (og collects it as resident
     * runs). BTR-4: zlib-compressed extents map ZFS-style {codec=kGzip,
     * psize} so the writer decodes at recovery time. */
    auto d = build_volume();
    /* compressed payload: 4100 'Z's, raw deflate (windowBits -15) */
    std::vector<u8> plain(4100, 'Z');
    std::vector<u8> comp;
    {
        uLongf cl = compressBound(static_cast<uLong>(plain.size())) + 64;
        comp.resize(cl);
        z_stream zs = {};
        deflateInit2(&zs, 5, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
        zs.next_in = plain.data();
        zs.avail_in = uInt(plain.size());
        zs.next_out = comp.data();
        zs.avail_out = uInt(cl);
        deflate(&zs, Z_FINISH);
        comp.resize(cl - zs.avail_out);
        deflateEnd(&zs);
    }
    memcpy(d.data() + L_DATA + 8192, comp.data(), comp.size());
    /* rebuild the FS-tree leaf: original rows + tiny.txt (inline) +
     * zlib.bin (compressed regular) */
    {
        NodeBuilder nb;
        const u64 MTIME = 1600000000;
        Key ki;
        ki.type = 1;
        ki.objectid = 256;
        nb.add(ki, make_inode(0x4000, NODESIZE, 2, MTIME));
        ki.objectid = 257;
        nb.add(ki, make_inode(0x8000, 14, 1, MTIME));
        ki.objectid = 258;
        nb.add(ki, make_inode(0x4000, NODESIZE, 2, MTIME));
        ki.objectid = 259;
        nb.add(ki, make_inode(0x8000, 22, 1, MTIME));
        ki.objectid = 262;
        nb.add(ki, make_inode(0x8000, 10, 1, MTIME)); /* tiny.txt */
        ki.objectid = 263;
        nb.add(ki, make_inode(0x8000, 4100, 1, MTIME)); /* zlib.bin */
        Key ke;
        ke.type = 108;
        ke.objectid = 257;
        ke.offset = 0;
        nb.add(ke, make_extent_data(L_DATA, 4096));
        ke.objectid = 259;
        nb.add(ke, make_extent_data(L_DATA + 4096, 4096));
        /* inline extent for tiny.txt */
        ke.objectid = 262;
        ke.offset = 0;
        {
            std::vector<u8> fe(21 + 10, 0);
            put64(fe.data() + 0, 1);
            put64(fe.data() + 8, 10);
            fe[20] = 0; /* INLINE */
            memcpy(fe.data() + 21, "TINYDATA!!", 10);
            nb.add(ke, fe);
        }
        /* zlib-compressed regular extent for zlib.bin */
        ke.objectid = 263;
        ke.offset = 0;
        {
            std::vector<u8> fe(53, 0);
            put64(fe.data() + 0, 1);    /* generation */
            put64(fe.data() + 8, 4100); /* ram_bytes */
            fe[16] = 1;                 /* compression: zlib */
            fe[20] = 1;                 /* type: regular */
            put64(fe.data() + 21, L_DATA + 8192);
            put64(fe.data() + 29, comp.size());
            put64(fe.data() + 37, 0);
            put64(fe.data() + 45, 4100);
            nb.add(ke, fe);
        }
        Key kd;
        kd.type = 84;
        kd.objectid = 256;
        kd.offset = 0x11110001;
        nb.add(kd, make_dir_item(257, "hello.txt", 1));
        kd.offset = 0x22220002;
        nb.add(kd, make_dir_item(258, "sub", 2));
        kd.offset = 0x33330003;
        nb.add(kd, make_dir_item(259, "nested.bin", 1));
        kd.offset = 0x44440004;
        nb.add(kd, make_dir_item(262, "tiny.txt", 1));
        kd.offset = 0x55550005;
        nb.add(kd, make_dir_item(263, "zlib.bin", 1));
        kd.objectid = 258;
        kd.offset = 0x66660006;
        nb.add(kd, make_dir_item(259, "nested.bin", 1));
        auto nd = nb.finish(L_FS_TREE, 0);
        memcpy(d.data() + L_FS_TREE, nd.data(), NODESIZE);
    }
    MemDevice dev(d);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    BtrfsScanStats stats;
    ASSERT_EQ(OK, scan_btrfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats))
        << err_text(0);
    std::vector<tree::FileNode*> all;
    std::function<void(tree::FileNode*)> walk = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            all.push_back(c);
            walk(c);
        }
    };
    walk(ra);
    tree::FileNode *tiny = nullptr, *zl = nullptr;
    for (auto* n : all) {
        if (n->props() && n->name_utf8() == "tiny.txt")
            tiny = n;
        if (n->props() && n->name_utf8() == "zlib.bin")
            zl = n;
    }
    fprintf(
        stderr,
        "DBG stats files=%llu dirs=%llu inline=%llu comp=%llu lzo=%llu nodes=%llu damaged=%llu\n",
        (unsigned long long)stats.files, (unsigned long long)stats.dirs,
        (unsigned long long)stats.inline_files, (unsigned long long)stats.compressed_extents,
        (unsigned long long)stats.lzo_extents, (unsigned long long)stats.nodes_read,
        (unsigned long long)stats.damaged_nodes);
    fprintf(stderr, "DBG tiny size=%llu extents=%zu resident=%zu\n",
            (unsigned long long)(tiny ? tiny->props()->logical_size : 0),
            tiny ? tiny->props()->extents.size() : 0,
            tiny ? tiny->props()->resident_data.size() : 0);
    ASSERT_NE(tiny, nullptr) << "inline file missing";
    ASSERT_NE(zl, nullptr) << "compressed file missing";
    /* BTR-3: inline content in resident_data */
    EXPECT_TRUE(tiny->props()->extents.empty());
    ASSERT_EQ(tiny->props()->resident_data.size(), size_t(10));
    EXPECT_EQ(0, memcmp(tiny->props()->resident_data.data(), "TINYDATA!!", 10));
    EXPECT_GE(stats.inline_files, 1u);
    /* BTR-4: zlib extent carries {codec=kGzip, psize} */
    ASSERT_EQ(zl->props()->extents.size(), 1u);
    EXPECT_EQ(zl->props()->extents[0].codec, u32(cr::compress::kGzip));
    EXPECT_EQ(zl->props()->extents[0].psize, u64(comp.size()));
    EXPECT_EQ(zl->props()->extents[0].length, 4100u);
    EXPECT_GE(stats.compressed_extents, 1u);
}

namespace cr {
namespace btrfs {

/* ---- S-005: on-disk UTF-8 names decode, never byte-lift ---------------- */

TEST(Btrfs, NonAsciiNamesDecodedNotByteLifted) {
    /* Btrfs dir-item names are raw UTF-8 bytes the og passes through
     * unconverted; the engine must decode them to true code points. */
    auto img = build_volume();
    {
        NodeBuilder nb;
        Key ki;
        ki.type = 1;       /* INODE_ITEM */
        ki.objectid = 256; /* subvol root dir */
        nb.add(ki, make_inode(0x4000, NODESIZE, 2, 1700000000ull));
        ki.objectid = 257;
        nb.add(ki, make_inode(0x8000, 14, 1, 1700000000ull));
        ki.objectid = 258;
        nb.add(ki, make_inode(0x4000, NODESIZE, 2, 1700000000ull));
        ki.objectid = 259;
        nb.add(ki, make_inode(0x8000, 14, 1, 1700000000ull));
        Key ke;
        ke.type = 1;
        ke.objectid = 257;
        ke.offset = 0;
        nb.add(ke, make_extent_data(L_DATA, 4096));
        ke.objectid = 259;
        nb.add(ke, make_extent_data(L_DATA, 4096));
        std::vector<u8> packed = make_dir_item(257, "\xe7\x85\xa7\xe7\x89\x87.jpg", 1);
        {
            /* distinct child objectids: the engine emits one node per child
             * object (hard-link names for one object collapse to a single
             * node), so each probe name owns its own inode */
            std::vector<u8> acc = make_dir_item(258, "caf\xc3\xa9.txt", 2);
            packed.insert(packed.end(), acc.begin(), acc.end());
            std::vector<u8> bad = make_dir_item(259, "\xff(bad", 1);
            packed.insert(packed.end(), bad.begin(), bad.end());
        }
        Key kd;
        kd.type = 84;
        kd.objectid = 256;
        kd.offset = 0x11110001;
        nb.add(kd, packed);
        auto nd = nb.finish(L_FS_TREE, 0);
        memcpy(img.data() + L_FS_TREE, nd.data(), NODESIZE);
    }
    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"F");
    BtrfsScanStats st;
    ASSERT_EQ(OK, scan_btrfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));

    std::map<std::string, std::u16string> names;
    for (tree::FileNode* c = ra->first_child(); c; c = c->next_sibling())
        for (tree::FileNode* x = c->first_child(); x; x = x->next_sibling())
            if (x->props())
                names[x->name_utf8()] = x->props()->name;
    ASSERT_TRUE(names.count("\xe7\x85\xa7\xe7\x89\x87.jpg") != 0);
    EXPECT_EQ(names["\xe7\x85\xa7\xe7\x89\x87.jpg"], std::u16string(u"\u7167\u7247.jpg"));
    ASSERT_TRUE(names.count("caf\xc3\xa9.txt") != 0);
    EXPECT_EQ(names["caf\xc3\xa9.txt"], std::u16string(u"caf\u00e9.txt"));
    /* M-010: invalid lead bytes are now LOSSLESS (U+E000 + byte) on
     * ext4/Btrfs - the og engines keep raw names; the map key is the
     * tree's UTF-8 projection of U+E0FF (EE 83 BF) */
    ASSERT_TRUE(names.count("\xee\x83\xbf(bad") != 0);
    EXPECT_EQ(names["\xee\x83\xbf(bad"], std::u16string(u"\uE0FF(bad"));
}

} // namespace btrfs
} // namespace cr

TEST(Btrfs, Latin1NameRoundTripsRawBytes) {
    /* M-010 (diff-to-fix): Btrfs dir-item names are raw bytes the og's
     * engine passes through UNCONVERTED (BtrfsSearchDLL imports no
     * conversion API; .rsf stores UTF-8). A Latin-1 name's bytes must
     * stay reconstructible: the lossless mapping yields U+E000 + byte
     * per invalid byte (bijective; unit - 0xE000 = 0xE9). */
    auto img = build_volume();
    {
        NodeBuilder nb;
        Key ki;
        ki.type = 1;
        ki.objectid = 256;
        nb.add(ki, make_inode(0x4000, NODESIZE, 2, 1700000000ull));
        ki.objectid = 259;
        nb.add(ki, make_inode(0x8000, 10, 1, 1700000000ull));
        Key ke;
        ke.type = 108;
        ke.objectid = 259;
        ke.offset = 0;
        nb.add(ke, make_extent_data(L_DATA, 4096));
        Key kd;
        kd.type = 84;
        kd.objectid = 256;
        kd.offset = 0x11110001;
        /* "r\xE9sum\xE9.txt" in Latin-1: 0xE9 is not valid UTF-8 */
        nb.add(kd, make_dir_item(259, "r\xE9sum\xE9.txt", 1));
        auto nd = nb.finish(L_FS_TREE, 0);
        memcpy(img.data() + L_FS_TREE, nd.data(), NODESIZE);
    }
    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"F");
    BtrfsScanStats st;
    ASSERT_EQ(OK, scan_btrfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));
    tree::FileNode* found = nullptr;
    for (tree::FileNode* c = ra->first_child(); c; c = c->next_sibling())
        for (tree::FileNode* x = c->first_child(); x; x = x->next_sibling())
            if (x->props() && x->props()->name.size() == 10 && x->props()->name[1] == u16(0xE0E9))
                found = x;
    ASSERT_NE(found, nullptr) << "the Latin-1-named file must surface";
    /* the exact lossless form: 'r' U+E0E9 's' 'u' 'm' U+E0E9 '.' 't' 'x' 't' */
    const std::u16string want = u"r\uE0E9sum\uE0E9.txt";
    EXPECT_EQ(want, found->props()->name);
    /* and the raw bytes are recoverable through the inverse */
    std::string back;
    ASSERT_TRUE(text::lossless_u16_to_utf8(found->props()->name, back));
    EXPECT_EQ(std::string("r\xE9sum\xE9.txt"), back);
}

} // namespace
} // namespace btrfs
} // namespace cr

/* ---- S-028: xattr recovery (user.DOSATTRIB / user.* records / the
 * btrfs.compression property tag) ---------------------------------------- */

namespace cr {
namespace btrfs {
namespace {

std::vector<u8> make_xattr_item(const char* name, const char* value) {
    /* XATTR items share the dir_item record shape (kernel shares the
     * struct; og BtrfsSearchDLL :3424-3441 parses it identically) */
    const u32 nlen = u32(strlen(name));
    const u32 vlen = u32(strlen(value));
    std::vector<u8> r(30 + nlen + vlen, 0);
    put64(r.data() + 0, 0); /* location: unused for xattrs */
    put64(r.data() + 17, 1);
    put16(r.data() + 25, u16(vlen)); /* data_len */
    put16(r.data() + 27, u16(nlen)); /* name_len */
    r[29] = 0;
    memcpy(r.data() + 30, name, nlen);
    memcpy(r.data() + 30 + nlen, value, vlen);
    return r;
}

} // namespace

TEST(Btrfs, XattrDosAttribApplied) {
    /* a Samba-shared volume stores the Windows attribute set in
     * user.DOSATTRIB ("0x20" = archive); the og surfaces the bit */
    auto d = build_volume();
    /* patch: rebuild the leaf with an added XATTR item for inode 257 */
    {
        const u64 MTIME = 1600000000;
        NodeBuilder nb;
        Key ki;
        ki.type = 1; /* INODE_ITEM */
        ki.objectid = 256;
        nb.add(ki, make_inode(0x4000, NODESIZE, 2, MTIME));
        ki.objectid = 257;
        nb.add(ki, make_inode(0x8000, 14, 1, MTIME));
        ki.objectid = 258;
        nb.add(ki, make_inode(0x4000, NODESIZE, 2, MTIME));
        ki.objectid = 259;
        nb.add(ki, make_inode(0x8000, 22, 1, MTIME));
        Key ke;
        ke.type = 108; /* EXTENT_DATA */
        ke.objectid = 257;
        ke.offset = 0;
        nb.add(ke, make_extent_data(L_DATA, 4096));
        ke.objectid = 259;
        nb.add(ke, make_extent_data(L_DATA + 4096, 4096));
        Key kx;
        kx.type = 24; /* XATTR (the og's 0x18) */
        kx.objectid = 257;
        kx.offset = 0;
        nb.add(kx, make_xattr_item("user.DOSATTRIB", "0x20"));
        kx.objectid = 259;
        nb.add(kx, make_xattr_item("user.DOSATTRIB", "0x2")); /* hidden + ... */
        Key kd;
        kd.type = 84; /* DIR_ITEM */
        kd.objectid = 256;
        kd.offset = 0x11110001;
        nb.add(kd, make_dir_item(257, "hello.txt", 1));
        kd.offset = 0x22220002;
        nb.add(kd, make_dir_item(258, "sub", 2));
        kd.objectid = 258;
        kd.offset = 0x33330003;
        nb.add(kd, make_dir_item(259, "nested.bin", 1));
        auto nd = nb.finish(L_FS_TREE, 0);
        memcpy(d.data() + L_FS_TREE, nd.data(), NODESIZE);
    }
    MemDevice dev(std::move(d));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    BtrfsScanStats st;
    ASSERT_EQ(OK, scan_btrfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));
    tree::FileNode* hello = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props() && c->name_utf8() == "hello.txt")
                hello = c;
            find(c);
        }
    };
    find(ra);
    ASSERT_NE(hello, nullptr);
    EXPECT_EQ(0x20u, hello->props()->attributes) << "the archive bit lands";
    EXPECT_EQ(0u, st.xattr_records) << "DOSATTRIB is applied, not a record";
}

TEST(Btrfs, UserXattrRecordsEmitted) {
    /* other user.* xattrs become per-file attribute records (the og's
     * per-file list; our shape: the N-011 ADS-children pattern) */
    auto d = build_volume();
    {
        const u64 MTIME = 1600000000;
        NodeBuilder nb;
        Key ki;
        ki.type = 1;
        ki.objectid = 256;
        nb.add(ki, make_inode(0x4000, NODESIZE, 2, MTIME));
        ki.objectid = 257;
        nb.add(ki, make_inode(0x8000, 14, 1, MTIME));
        ki.objectid = 258;
        nb.add(ki, make_inode(0x4000, NODESIZE, 2, MTIME));
        ki.objectid = 259;
        nb.add(ki, make_inode(0x8000, 22, 1, MTIME));
        Key ke;
        ke.type = 108;
        ke.objectid = 257;
        ke.offset = 0;
        nb.add(ke, make_extent_data(L_DATA, 4096));
        ke.objectid = 259;
        nb.add(ke, make_extent_data(L_DATA + 4096, 4096));
        Key kx;
        kx.type = 24;
        kx.objectid = 257;
        kx.offset = 0;
        nb.add(kx, make_xattr_item("user.custom_tag", "val"));
        kx.offset = 1;
        nb.add(kx, make_xattr_item("user.mtime", "1234567890"));
        Key kd;
        kd.type = 84;
        kd.objectid = 256;
        kd.offset = 0x11110001;
        nb.add(kd, make_dir_item(257, "hello.txt", 1));
        kd.offset = 0x22220002;
        nb.add(kd, make_dir_item(258, "sub", 2));
        kd.objectid = 258;
        kd.offset = 0x33330003;
        nb.add(kd, make_dir_item(259, "nested.bin", 1));
        auto nd = nb.finish(L_FS_TREE, 0);
        memcpy(d.data() + L_FS_TREE, nd.data(), NODESIZE);
    }
    MemDevice dev(std::move(d));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    BtrfsScanStats st;
    ASSERT_EQ(OK, scan_btrfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));
    tree::FileNode* hello = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props() && c->name_utf8() == "hello.txt")
                hello = c;
            find(c);
        }
    };
    find(ra);
    ASSERT_NE(hello, nullptr);
    /* two ..xattr children carrying the values as resident content */
    std::set<std::string> xn;
    for (tree::FileNode* c = hello->first_child(); c; c = c->next_sibling())
        if (c->props())
            xn.insert(c->name_utf8());
    EXPECT_TRUE(xn.count("..xattr/user.custom_tag"));
    EXPECT_TRUE(xn.count("..xattr/user.mtime"));
    EXPECT_EQ(2u, st.xattr_records);
    for (tree::FileNode* c = hello->first_child(); c; c = c->next_sibling()) {
        if (c->props() && c->name_utf8() == "..xattr/user.custom_tag") {
            ASSERT_EQ(size_t(3), c->props()->resident_data.size());
            EXPECT_EQ(0, memcmp(c->props()->resident_data.data(), "val", 3));
        }
    }
}

TEST(Btrfs, CompressionPropertyTagged) {
    /* btrfs.compression: {"zlib"->1, "zlo"->2, "zstd"->3}; the property is
     * metadata ALONGSIDE the per-extent truth (I-014 B4: the og's
     * property source and our extent source differ - ours stays truth) */
    auto d = build_volume();
    {
        const u64 MTIME = 1600000000;
        NodeBuilder nb;
        Key ki;
        ki.type = 1;
        ki.objectid = 256;
        nb.add(ki, make_inode(0x4000, NODESIZE, 2, MTIME));
        ki.objectid = 257;
        nb.add(ki, make_inode(0x8000, 14, 1, MTIME));
        ki.objectid = 258;
        nb.add(ki, make_inode(0x4000, NODESIZE, 2, MTIME));
        ki.objectid = 259;
        nb.add(ki, make_inode(0x8000, 22, 1, MTIME));
        Key ke;
        ke.type = 108;
        ke.objectid = 257;
        ke.offset = 0;
        nb.add(ke, make_extent_data(L_DATA, 4096));
        ke.objectid = 259;
        nb.add(ke, make_extent_data(L_DATA + 4096, 4096));
        Key kx;
        kx.type = 24;
        kx.objectid = 257; /* the property rides an XATTR item */
        kx.offset = 0;
        nb.add(kx, make_xattr_item("btrfs.compression", "zstd"));
        Key kd;
        kd.type = 84;
        kd.objectid = 256;
        kd.offset = 0x11110001;
        nb.add(kd, make_dir_item(257, "hello.txt", 1));
        kd.offset = 0x22220002;
        nb.add(kd, make_dir_item(258, "sub", 2));
        kd.objectid = 258;
        kd.offset = 0x33330003;
        nb.add(kd, make_dir_item(259, "nested.bin", 1));
        auto nd = nb.finish(L_FS_TREE, 0);
        memcpy(d.data() + L_FS_TREE, nd.data(), NODESIZE);
    }
    MemDevice dev(std::move(d));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    BtrfsScanStats st;
    ASSERT_EQ(OK, scan_btrfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));
    EXPECT_EQ(1u, st.compression_property_files) << "the zstd property tag counted";
    /* the per-extent truth: the fixture's extent is UNcompressed - the
     * property is metadata, the extent codec stays 0 (raw) */
    tree::FileNode* hello = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props() && c->name_utf8() == "hello.txt")
                hello = c;
            find(c);
        }
    };
    find(ra);
    ASSERT_NE(hello, nullptr);
    ASSERT_FALSE(hello->props()->extents.empty());
    EXPECT_EQ(0u, hello->props()->extents[0].codec);
}

} // namespace btrfs
} // namespace cr
