/* test_hfsplus_volume.cpp - synthetic HFS+ volume end-to-end.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/hfsplus/cr_hfsplus_engine.h"
#include <zlib.h>
#include "cr/compress/cr_decomp.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <cstring>
#include <set>

namespace cr {
namespace hfsplus {
namespace {

constexpr u32 BS = 4096; /* HFS+ block size */

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

void be16(u8* p, u16 v) {
    p[0] = u8(v >> 8);
    p[1] = u8(v);
}
void be32(u8* p, u32 v) {
    p[0] = u8(v >> 24);
    p[1] = u8(v >> 16);
    p[2] = u8(v >> 8);
    p[3] = u8(v);
}
void be64(u8* p, u64 v) {
    be32(p, u32(v >> 32));
    be32(p + 4, u32(v));
}

/* blocks: 0..1 = boot + VH (VH at byte 1024 inside blocks 0-1), 2 = catalog
 * header node, 3 = catalog leaf 1 (live), 4 = catalog leaf 2 (freed, holds
 * stale record), 5 = allocation bitmap, 6+ = file data */
constexpr u32 B_CAT_HDR = 2;
constexpr u32 B_CAT_LEAF1 = 3;
constexpr u32 B_CAT_LEAF2 = 4;
constexpr u32 B_ALLOC = 5;
constexpr u32 B_FILE_DATA = 6;
constexpr u32 B_RSRC_DATA = 7;
constexpr u32 B_GONE_DATA = 8;

constexpr u32 NODE_SIZE = 4096; /* one block per node */

void write_fork(u8* p, u64 logical, u32 start_block, u32 block_count) {
    be64(p + 0x00, logical);
    be32(p + 0x08, BS); /* clump */
    be32(p + 0x0C, block_count);
    be32(p + 0x10, start_block);
    be32(p + 0x14, block_count);
    /* 7 more extents: zero */
}

/* Build one catalog leaf record: key + file/folder body. Returns bytes. */
std::vector<u8> make_record(u32 parent, const std::u16string& name, i16 type, u32 cnid,
                            u64 data_size, u32 data_block, u32 data_blocks, u64 rsrc_size,
                            u32 rsrc_block, u32 rsrc_blocks) {
    std::vector<u8> key;
    const u32 name_units = u32(name.size());
    const u32 key_len = u32(2 + 4 + 2 + name_units * 2);
    key.resize(key_len, 0);
    be16(key.data(), u16(key_len - 2)); /* keyLength EXCLUDES itself */
    be32(key.data() + 2, parent);
    be16(key.data() + 6, u16(name_units));
    for (u32 i = 0; i < name_units; ++i)
        be16(key.data() + 8 + u32(i) * 2, name[i]);

    std::vector<u8> body;
    if (type == 2) { /* file */
        body.assign(0xF8, 0);
        be16(body.data() + 0, 2); /* recordType */
        be32(body.data() + 8, cnid);
        be32(body.data() + 0x0C, 1000000); /* create */
        be32(body.data() + 0x10, 2000000); /* contentMod */
        be32(body.data() + 0x14, 3000000); /* attrMod */
        be32(body.data() + 0x18, 4000000); /* access */
        write_fork(body.data() + 0x58, data_size, data_block, data_blocks);
        write_fork(body.data() + 0xA8, rsrc_size, rsrc_block, rsrc_blocks);
    } else { /* folder */
        body.assign(0x58, 0);
        be16(body.data() + 0, 1); /* recordType */
        be32(body.data() + 4, 3); /* valence */
        be32(body.data() + 8, cnid);
        be32(body.data() + 0x0C, 1000000);
        be32(body.data() + 0x10, 2000000);
    }
    std::vector<u8> out = key;
    if (out.size() & 1)
        out.push_back(0); /* key padded to even */
    out.insert(out.end(), body.begin(), body.end());
    return out;
}

/* Assemble a leaf node from records.
 * A-005 (diff-to-fix): the on-disk layout (TN1150; the og's journal-image
 * validator FUN_180010720 hard-gates this shape) packs records UP from
 * offset 14 - record 0 at [14, 14+l0), record 1 after it - with the
 * record-offset array growing DOWN from the node end: [end-2] = record
 * 0's offset (= 14), [end-4] = record 1's offset, ..., the deepest entry
 * [end-2*(n+1)] = the free-space start. The pre-fix builder packed
 * records DOWN from below the array (self-consistent with the inverted
 * reader, wrong vs the format). */
std::vector<u8> make_leaf_node(const std::vector<std::vector<u8>>& recs, u32 forward,
                               u32 node_size = NODE_SIZE) {
    std::vector<u8> node(node_size, 0);
    be32(node.data() + 0, forward); /* forwardLink */
    be32(node.data() + 4, 0);       /* backwardLink */
    node[8] = 0xFF;                 /* kind = -1 (kBTLeafNode, TN1150) */
    node[9] = 1;                    /* height */
    be16(node.data() + 10, u16(recs.size()));
    u32 at = 14; /* first record right after the 14-byte node header */
    std::vector<u32> offsets;
    for (const auto& r : recs) {
        memcpy(node.data() + at, r.data(), r.size());
        offsets.push_back(at);
        at += u32(r.size());
    }
    /* offset array: entry i at nodeSize - 2*(i+1); the deepest entry
     * (n+1) is the free-space start */
    for (u32 i = 0; i < offsets.size(); ++i)
        be16(node.data() + node_size - (i + 1) * 2, u16(offsets[i]));
    be16(node.data() + node_size - (offsets.size() + 1) * 2, u16(at));
    return node;
}

std::vector<u8> build_volume() {
    std::vector<u8> d(16 * BS, 0);

    /* volume header at byte 1024 (inside block 0) */
    u8* vh = d.data() + 1024;
    be16(vh + 0x00, 0x482B);                             /* H+ */
    be16(vh + 0x02, 4);                                  /* version */
    be32(vh + 0x0C, 0);                                  /* journalInfoBlock: none */
    be32(vh + 0x20, 2);                                  /* fileCount (2 live files) */
    be32(vh + 0x24, 2);                                  /* folderCount */
    be32(vh + 0x28, BS);                                 /* blockSize */
    be32(vh + 0x2C, 16);                                 /* totalBlocks */
    be32(vh + 0x30, 6);                                  /* freeBlocks */
    be32(vh + 0x40, 100);                                /* nextCatalogID */
    write_fork(vh + 0x70, 2 * BS, B_ALLOC, 2);           /* allocation file */
    write_fork(vh + 0xC0, 0, 0, 0);                      /* extents overflow: empty */
    write_fork(vh + 0x110, 3 * NODE_SIZE, B_CAT_HDR, 3); /* catalog: 3 nodes */
    write_fork(vh + 0x160, 0, 0, 0);                     /* attributes: empty */

    /* catalog header node (node 0): BTHeaderRec at +14 (TN1150 layout) */
    u8* hdr = d.data() + u64(B_CAT_HDR) * BS;
    be32(hdr + 0, B_CAT_LEAF1);       /* forwardLink -> leaf 1 (node idx 1) */
    hdr[8] = 1;                       /* kind = 1 (kBTHeaderNode, TN1150) */
    hdr[9] = 0;                       /* height 0 (og FUN_180010720 requires) */
    be16(hdr + 10, 3);                /* numRecords */
    be16(hdr + 14 + 0x00, 1);         /* treeDepth */
    be32(hdr + 14 + 0x02, 1);         /* rootNode = leaf 1 */
    be32(hdr + 14 + 0x06, 4);         /* leafRecords */
    be32(hdr + 14 + 0x0A, 1);         /* firstLeafNode = node 1 */
    be32(hdr + 14 + 0x0E, 1);         /* lastLeafNode = node 1 */
    be16(hdr + 14 + 0x12, NODE_SIZE); /* nodeSize */
    be32(hdr + 14 + 0x16, 3);         /* totalNodes */
    be32(hdr + 14 + 0x1A, 1);         /* freeNodes (node 2 freed) */

    /* leaf 1 (live): root folder, a child file, a subfolder, nested file */
    auto root_rec = make_record(1, u"Macintosh HD", 1, 2, 0, 0, 0, 0, 0, 0);
    auto file_rec = make_record(2, u"readme.txt", 2, 20, 26, B_FILE_DATA, 1, 0, 0, 0);
    auto dir_rec = make_record(2, u"Documents", 1, 21, 0, 0, 0, 0, 0, 0);
    auto nested_rec = make_record(21, u"notes.rtf", 2, 22, 44, B_RSRC_DATA, 1, 8, B_GONE_DATA, 1);
    auto leaf1 = make_leaf_node({root_rec, file_rec, dir_rec, nested_rec}, 0 /* last */);
    memcpy(d.data() + u64(B_CAT_LEAF1) * BS, leaf1.data(), NODE_SIZE);

    /* leaf 2 (FREED - not in chain): stale record of a deleted file */
    auto gone_rec = make_record(2, u"deleted.bin", 2, 30, 512, B_GONE_DATA, 1, 0, 0, 0);
    auto leaf2 = make_leaf_node({gone_rec}, 0);
    memcpy(d.data() + u64(B_CAT_LEAF2) * BS, leaf2.data(), NODE_SIZE);

    /* allocation bitmap: MSB-first, blocks 0..9 in use, 10..15 free */
    u8* am = d.data() + u64(B_ALLOC) * BS;
    am[0] = 0xFF; /* blocks 0-7 */
    am[1] = 0xC0; /* blocks 8,9 in use; 10-15 free */

    /* file data */
    memcpy(d.data() + u64(B_FILE_DATA) * BS, "readme content here!!", 22);
    memcpy(d.data() + u64(B_RSRC_DATA) * BS, "notes resource fork data", 25);
    memset(d.data() + u64(B_GONE_DATA) * BS, 0x5A, 512);
    return d;
}

TEST(HfsPlus, VolumeHeaderValidation) {
    std::vector<u8> d(2 * BS, 0);
    u8* vh = d.data() + 1024;
    be16(vh + 0x00, 0x482B);
    be16(vh + 0x02, 4);
    be32(vh + 0x28, 4096);
    be32(vh + 0x2C, 100);
    VolumeHeader h;
    EXPECT_TRUE(validate_volume_header(vh, h));
    EXPECT_EQ(4096u, h.block_size);

    /* bad signature */
    be16(vh + 0x00, 0x482A);
    EXPECT_FALSE(validate_volume_header(vh, h));
    be16(vh + 0x00, 0x4858); /* HX accepted */
    EXPECT_TRUE(validate_volume_header(vh, h));
    be16(vh + 0x00, 0x482B);

    /* version 17+ rejected */
    be16(vh + 0x02, 17);
    EXPECT_FALSE(validate_volume_header(vh, h));
    be16(vh + 0x02, 4);

    /* block size not multiple of 512 */
    be32(vh + 0x28, 1000);
    EXPECT_FALSE(validate_volume_header(vh, h));
    be32(vh + 0x28, 4096);

    /* M-142: the og never gates the fork fields at the VH gate
     * (FUN_18000c6e0 swaps freeBlocks/journalInfoBlock, never the forks;
     * FUN_18000cf90 - the next stage - is the fork byte-swapper, no
     * compares). A torn allocation fork is ACCEPTED. */
    write_fork(vh + 0x70, 999 * BS, 0, 200);
    EXPECT_TRUE(validate_volume_header(vh, h));
    write_fork(vh + 0x70, 0, 0, 0);
}

TEST(HfsPlus, VhFreeBlocksAboveTotalRejected) {
    /* M-142 fixture A (the FP direction): the og rejects freeBlocks >
     * totalBlocks (FUN_18000c6e0 :10005-10006, local_1e8 <= uStack_1ec)
     * even with valid forks/sig/version/blockSize - the old core accepted
     * this header because its fork clamps passed. */
    std::vector<u8> d(2 * BS, 0);
    u8* vh = d.data() + 1024;
    be16(vh + 0x00, 0x482B);
    be16(vh + 0x02, 4);
    be32(vh + 0x28, 4096);
    be32(vh + 0x2C, 0x10000);
    be32(vh + 0x30, 0xFFFFFFFF); /* freeBlocks > totalBlocks */
    VolumeHeader h;
    EXPECT_FALSE(validate_volume_header(vh, h))
        << "freeBlocks above totalBlocks is the og's rejection (M-142 A)";
}

TEST(HfsPlus, VhJournalInfoBlockAboveTotalRejected) {
    /* M-142: journalInfoBlock > totalBlocks is the og's second clamp
     * (uStack_20c <= uStack_1ec) - the only u32s it swaps besides
     * blockSize. */
    std::vector<u8> d(2 * BS, 0);
    u8* vh = d.data() + 1024;
    be16(vh + 0x00, 0x482B);
    be16(vh + 0x02, 4);
    be32(vh + 0x28, 4096);
    be32(vh + 0x2C, 0x10000);
    be32(vh + 0x0C, 0x10001); /* journalInfoBlock > totalBlocks */
    VolumeHeader h;
    EXPECT_FALSE(validate_volume_header(vh, h))
        << "journalInfoBlock above totalBlocks is the og's rejection";
}

TEST(HfsPlus, VhTornAllocForkStillValidates) {
    /* M-142 fixture B (the FN direction): allocationFile.total_blocks
     * torn to 0xFFFFFFFF with everything else valid - the og scans this
     * volume (forks are swapped by FUN_18000d030's fork walker but never
     * gated); the old core refused the whole volume. */
    std::vector<u8> d(2 * BS, 0);
    u8* vh = d.data() + 1024;
    be16(vh + 0x00, 0x482B);
    be16(vh + 0x02, 4);
    be32(vh + 0x28, 4096);
    be32(vh + 0x2C, 0x10000);
    be32(vh + 0x30, 8); /* freeBlocks sane */
    write_fork(vh + 0x70, 999 * BS, 0, 0xFFFFFFFF); /* torn alloc fork */
    write_fork(vh + 0xC0, 0, 0, 0x7FFFFFFF);        /* torn extents fork */
    VolumeHeader h;
    EXPECT_TRUE(validate_volume_header(vh, h))
        << "torn fork block counts never gate the VH (M-142 B)";
}

TEST(HfsPlus, VhFreeBlocksEqualTotalAccepted) {
    /* boundary: freeBlocks == totalBlocks (a completely full volume) is
     * the og's <= edge - accepted. */
    std::vector<u8> d(2 * BS, 0);
    u8* vh = d.data() + 1024;
    be16(vh + 0x00, 0x4858); /* HX */
    be16(vh + 0x02, 4);
    be32(vh + 0x28, 4096);
    be32(vh + 0x2C, 0x10000);
    be32(vh + 0x30, 0x10000); /* == totalBlocks */
    be32(vh + 0x0C, 0x10000); /* journalInfoBlock == totalBlocks */
    VolumeHeader h;
    EXPECT_TRUE(validate_volume_header(vh, h))
        << "both ==-edges accepted (the <= forms)";
}

TEST(HfsPlus, EndToEndWalk) {
    auto img = build_volume();
    MemDevice dev(img);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");

    HfsScanFlags fl;
    HfsScanStats st;
    u32 rc = scan_hfsplus(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(2u, st.files);
    EXPECT_EQ(2u, st.dirs);
    EXPECT_EQ(4u, st.leaf_records);
    EXPECT_EQ(1u, st.orphans);

    /* collect full paths from the live tree */
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

    EXPECT_TRUE(paths.count("Macintosh HD"));
    EXPECT_TRUE(paths.count("Macintosh HD/readme.txt"));
    EXPECT_TRUE(paths.count("Macintosh HD/Documents"));
    EXPECT_TRUE(paths.count("Macintosh HD/Documents/notes.rtf"));
    /* resource fork child of a file with rsrc data */
    EXPECT_TRUE(paths.count("Macintosh HD/Documents/notes.rtf/..namedfork/rsrc"));

    /* deleted orphan record from the freed node */
    int deleted_count = 0;
    for (tree::FileNode* c = rd->first_child(); c; c = c->next_sibling()) {
        if (!c->props())
            continue;
        ++deleted_count;
        EXPECT_EQ("deleted.bin", c->name_utf8());
        EXPECT_EQ(u64(512), c->props()->logical_size);
        EXPECT_EQ(u64(B_GONE_DATA * BS), c->props()->extents[0].offset);
        /* blocks 8-9 in use in the bitmap: data block B_GONE_DATA=8 is IN
         * USE (marked allocated) -> reused/overwritten grading */
    }
    EXPECT_EQ(1, deleted_count);

    /* file size + extents of the live file */
    for (tree::FileNode* c = ra->first_child(); c; c = c->next_sibling())
        for (tree::FileNode* cc = c->first_child(); cc; cc = cc->next_sibling())
            if (cc->name_utf8() == "readme.txt") {
                EXPECT_EQ(u64(26), cc->props()->logical_size);
                ASSERT_EQ(1u, cc->props()->extents.size());
                EXPECT_EQ(u64(B_FILE_DATA * BS), cc->props()->extents[0].offset);
            }
}

/* ---- S-002: attributes B-tree + decmpfs -------------------------------- */

/* one attributes-B-tree leaf record: key + inline data wrapper */
std::vector<u8> make_attr_record(u32 cnid, const std::u16string& name, u32 record_type,
                                 const std::vector<u8>& value) {
    const u32 n = u32(name.size());
    const u32 key_len = 14 + 2 * n; /* keyLength COUNTS itself (og :7771) */
    std::vector<u8> rec;
    rec.resize(key_len, 0);
    be16(rec.data(), u16(key_len)); /* includes itself */
    be32(rec.data() + 2, 0);        /* pad */
    be32(rec.data() + 4, cnid);     /* fileId (og: +4) */
    be32(rec.data() + 8, 0);        /* startBlock */
    be16(rec.data() + 12, u16(n));  /* nameLen in UTF-16 chars (og: +0xC) */
    for (u32 i = 0; i < n; ++i)
        be16(rec.data() + 14 + u32(i) * 2, name[i]);
    /* data: [recordType BE][r1][r2][length BE][value] (og gate 0x10) */
    const u32 data_len = 16 + u32(value.size());
    rec.resize(key_len + data_len, 0);
    be32(rec.data() + key_len, record_type);
    be32(rec.data() + key_len + 12, u32(value.size()));
    if (!value.empty())
        memcpy(rec.data() + key_len + 16, value.data(), value.size());
    return rec;
}

void put32le(u8* p, u32 v) {
    p[0] = u8(v);
    p[1] = u8(v >> 8);
    p[2] = u8(v >> 16);
    p[3] = u8(v >> 24);
}

std::vector<u8> zlib_compress(const std::string& plain) {
    uLongf bound = compressBound(uLong(plain.size()));
    std::vector<u8> out(bound + 16, 0);
    uLongf out_len = bound;
    EXPECT_EQ(Z_OK, compress2(out.data(), &out_len, reinterpret_cast<const Bytef*>(plain.data()),
                              uLong(plain.size()), 6));
    out.resize(out_len);
    return out;
}

constexpr u32 B_ATTR_HDR = 16;
constexpr u32 B_ATTR_LEAF = 17;
constexpr u32 B_MODE4_RSRC = 20;
constexpr u32 ATTRS_TOTAL_BLOCKS = 32;

/* decmpfs value (LITTLE-endian x86 structs, og FUN_180009c10 :7841 native load) */
std::vector<u8> decmpfs_value(u32 mode, u32 size, u32 rsrc_off) {
    std::vector<u8> v;
    v.resize(mode == 3 ? 12 : 16, 0);
    put32le(v.data(), 0x66706D63); /* 'cmpf' LE */
    put32le(v.data() + 4, mode);
    put32le(v.data() + 8, size);
    if (mode == 4)
        put32le(v.data() + 12, rsrc_off);
    return v;
}

TEST(HfsPlus, DecmpfsInlineAndRsrcContentRecovered) {
    auto img = build_volume();
    img.resize(ATTRS_TOTAL_BLOCKS * BS, 0);
    u8* vh = img.data() + 1024;
    be32(vh + 0x2C, ATTRS_TOTAL_BLOCKS);
    write_fork(vh + 0x160, 2 * NODE_SIZE, B_ATTR_HDR, 2);

    const std::string content25 = "inline compressed pages content - decmpfs mode 3 payload";
    const std::string content26 = "resource-fork compressed payload - decmpfs mode 4";
    const std::vector<u8> z25 = zlib_compress(content25);
    const std::vector<u8> z26 = zlib_compress(content26);

    /* catalog leaf 1 rebuilt with two empty-data-fork files */
    auto root_rec = make_record(1, u"Macintosh HD", 1, 2, 0, 0, 0, 0, 0, 0);
    auto file_rec = make_record(2, u"readme.txt", 2, 20, 26, B_FILE_DATA, 1, 0, 0, 0);
    auto dir_rec = make_record(2, u"Documents", 1, 21, 0, 0, 0, 0, 0, 0);
    auto nested_rec = make_record(21, u"notes.rtf", 2, 22, 44, B_RSRC_DATA, 1, 8, B_GONE_DATA, 1);
    auto inline_rec = make_record(2, u"inline.pages", 2, 25, 0, 0, 0, 0, 0, 0);
    auto rsrc_rec = make_record(2, u"rsrc.pages", 2, 26, 0, 0, 0, 64 + z26.size(), B_MODE4_RSRC, 1);
    auto leaf1 = make_leaf_node({root_rec, file_rec, dir_rec, nested_rec, inline_rec, rsrc_rec}, 0);
    memcpy(img.data() + u64(B_CAT_LEAF1) * BS, leaf1.data(), NODE_SIZE);
    /* header leafRecords count is informational; nothing gates on it */

    /* attributes tree: header + one leaf */
    {
        u8* hdr = img.data() + u64(B_ATTR_HDR) * BS;
        be32(hdr + 0, B_ATTR_LEAF);
        hdr[8] = 1; /* kBTHeaderNode */
        hdr[9] = 0;
        be16(hdr + 10, 3);
        be32(hdr + 14 + 0x0A, 1); /* firstLeaf */
        be32(hdr + 14 + 0x0E, 1); /* lastLeaf */
        be16(hdr + 14 + 0x12, NODE_SIZE);
    }
    std::vector<u8> value25 = decmpfs_value(3, u32(content25.size()), 0);
    value25.insert(value25.end(), z25.begin(), z25.end());
    const std::vector<u8> tag_value = {'t', 'a', 'g', '-', 'v', 'a', 'l', 'u', 'e'};
    std::vector<u8> value26 = decmpfs_value(4, u32(content26.size()), 64);
    auto a25 = make_attr_record(25, u"com.apple.decmpfs", 0x10, value25);
    auto atag = make_attr_record(22, u"com.apple.custom:tag", 0x10, tag_value);
    auto a26 = make_attr_record(26, u"com.apple.decmpfs", 0x10, value26);
    auto abad = make_attr_record(20, u"com.apple.bad", 0x20, {1, 2, 3});
    auto aleaf = make_leaf_node({a25, atag, a26, abad}, 0);
    memcpy(img.data() + u64(B_ATTR_LEAF) * BS, aleaf.data(), NODE_SIZE);

    /* mode-4 resource fork payload: 64 bytes junk, zlib stream at +64 */
    memset(img.data() + u64(B_MODE4_RSRC) * BS, 0xAB, 64);
    memcpy(img.data() + u64(B_MODE4_RSRC) * BS + 64, z26.data(), z26.size());

    /* locate the value payloads inside the assembled leaf for the
     * expected-extent math (the record offset array packs downward) */
    auto find_in_leaf = [&](const std::vector<u8>& needle) -> u64 {
        const u8* b = aleaf.data();
        const u8* f = std::search(b, b + aleaf.size(), needle.begin(), needle.end());
        EXPECT_NE(f, b + aleaf.size());
        return u64(f - b);
    };
    const u64 v25_in_leaf = find_in_leaf(value25);
    const u64 tag_in_leaf = find_in_leaf(tag_value);

    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    HfsScanFlags fl;
    HfsScanStats st;
    u32 rc = scan_hfsplus(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(2u, st.decmpfs_files);
    EXPECT_EQ(1u, st.named_forks);
    EXPECT_EQ(1u, st.attr_unsupported);
    EXPECT_EQ(3u, st.attr_records); /* collected attrs (the unsupported
                                       record is skipped, og parity) */

    tree::FileNode *inl = nullptr, *rsrc = nullptr, *tagf = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            const std::string nm = c->name_utf8();
            if (nm == "inline.pages")
                inl = c;
            else if (nm == "rsrc.pages")
                rsrc = c;
            else if (nm == "..namedfork/com.apple.custom:tag")
                tagf = c;
            find(c);
        }
    };
    find(ra);

    /* mode 3: codec extent pointing into the attributes-B-tree node */
    ASSERT_NE(inl, nullptr) << "compressed file missing from results";
    ASSERT_NE(nullptr, inl->props());
    EXPECT_EQ(u64(content25.size()), inl->props()->logical_size);
    ASSERT_EQ(1u, inl->props()->extents.size());
    {
        const auto& e = inl->props()->extents[0];
        EXPECT_EQ(u32(compress::kZlib), e.codec);
        EXPECT_EQ(u64(B_ATTR_LEAF) * BS + v25_in_leaf + 12, e.offset);
        EXPECT_EQ(u64(content25.size()), e.length);
        EXPECT_EQ(u64(value25.size()) - 12, e.psize);
        /* the WRITER contract end-to-end: read the extent bytes from the
         * image and inflate - the recovered content must be exact */
        std::vector<u8> packed(size_t(e.psize), 0), plain(size_t(e.length), 0);
        memcpy(packed.data(), img.data() + e.offset, size_t(e.psize));
        ASSERT_TRUE(compress::decompress_block(e.codec, packed.data(), packed.size(), plain.data(),
                                               plain.size()));
        EXPECT_EQ(0, memcmp(plain.data(), content25.data(), content25.size()));
    }

    /* mode 4: codec extent into the resource fork at the recorded offset */
    ASSERT_NE(rsrc, nullptr);
    ASSERT_EQ(1u, rsrc->props()->extents.size());
    {
        const auto& e = rsrc->props()->extents[0];
        EXPECT_EQ(u32(compress::kZlib), e.codec);
        EXPECT_EQ(u64(B_MODE4_RSRC) * BS + 64, e.offset);
        EXPECT_EQ(u64(content26.size()), e.length);
        std::vector<u8> packed(size_t(e.psize), 0), plain(size_t(e.length), 0);
        memcpy(packed.data(), img.data() + e.offset, size_t(e.psize));
        ASSERT_TRUE(compress::decompress_block(e.codec, packed.data(), packed.size(), plain.data(),
                                               plain.size()));
        EXPECT_EQ(0, memcmp(plain.data(), content26.data(), content26.size()));
    }

    /* named attribute -> ..namedfork/<name> child with the raw value */
    ASSERT_NE(tagf, nullptr);
    ASSERT_EQ(1u, tagf->props()->extents.size());
    EXPECT_EQ(u64(tag_value.size()), tagf->props()->logical_size);
    EXPECT_EQ(u64(B_ATTR_LEAF) * BS + tag_in_leaf, tagf->props()->extents[0].offset);
    EXPECT_EQ(0, memcmp(img.data() + tagf->props()->extents[0].offset, tag_value.data(),
                        tag_value.size()));
}

TEST(HfsPlus, DecmpfsDisabledLeavesEmptyShell) {
    /* the flag-off mode keeps the pre-S-002 shape: no attribute walk
     * effects, empty data fork stays an empty node */
    auto img = build_volume();
    img.resize(ATTRS_TOTAL_BLOCKS * BS, 0);
    u8* vh = img.data() + 1024;
    be32(vh + 0x2C, ATTRS_TOTAL_BLOCKS);
    write_fork(vh + 0x160, 2 * NODE_SIZE, B_ATTR_HDR, 2);
    const std::string content = "compressed";
    std::vector<u8> value = decmpfs_value(3, u32(content.size()), 0);
    const std::vector<u8> z = zlib_compress(content);
    value.insert(value.end(), z.begin(), z.end());
    {
        u8* hdr = img.data() + u64(B_ATTR_HDR) * BS;
        hdr[8] = 1;
        be16(hdr + 14 + 0x12, NODE_SIZE);
        be32(hdr + 14 + 0x0A, 1);
        be32(hdr + 14 + 0x0E, 1);
        auto rec = make_attr_record(20, u"com.apple.decmpfs", 0x10, value);
        auto leaf = make_leaf_node({rec}, 0);
        memcpy(img.data() + u64(B_ATTR_LEAF) * BS, leaf.data(), NODE_SIZE);
    }
    auto file_rec2 = make_record(2, u"readme.txt", 2, 20, 0, 0, 0, 0, 0, 0);
    auto root_rec2 = make_record(1, u"Macintosh HD", 1, 2, 0, 0, 0, 0, 0, 0);
    auto leaf1 = make_leaf_node({root_rec2, file_rec2}, 0);
    memcpy(img.data() + u64(B_CAT_LEAF1) * BS, leaf1.data(), NODE_SIZE);

    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    HfsScanFlags fl;
    fl.resolve_decmpfs = false;
    fl.named_forks = false;
    HfsScanStats st;
    ASSERT_EQ(OK,
              scan_hfsplus(&dev, fl, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));
    EXPECT_EQ(0u, st.decmpfs_files);
    tree::FileNode* f = nullptr;
    for (tree::FileNode* c = ra->first_child(); c; c = c->next_sibling())
        for (tree::FileNode* x = c->first_child(); x; x = x->next_sibling())
            if (x->name_utf8() == "readme.txt")
                f = x;
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(0u, f->props()->extents.size());
    EXPECT_EQ(0u, f->props()->logical_size);
}

TEST(HfsPlus, MacTimeConversion) {
    /* Mac epoch 1904-01-01 00:00:00 == filetime 1904 delta */
    EXPECT_EQ(9561628800ULL * 10000000ULL, mac_to_filetime(0));
    /* unix epoch (1970) = 2082844800 mac seconds */
    EXPECT_EQ(11644473600ULL * 10000000ULL, mac_to_filetime(2082844800));
}

TEST(HfsPlus, BadSignatureRejected) {
    std::vector<u8> d(4 * BS, 0);
    MemDevice dev(d);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"F");
    HfsScanFlags fl;
    HfsScanStats st;
    EXPECT_EQ(ERR_BAD_MAGIC,
              scan_hfsplus(&dev, fl, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));
}

/* S-021 fixture helpers live with the landed tests below (build_volume_journal / write_tx_entry /
 * write_journal_info) */

u32 rd16(const u8* p) {
    return u32(p[0]) << 8 | p[1];
}

TEST(HfsPlus, LeafNodeLayoutMatchesOnDiskFormat) {
    /* A-005 guard: fixture nodes must carry the on-disk shape the og's
     * journal-image validator (FUN_180010720) accepts - record 0's
     * offset == 14 at [end-2], ascending offsets deeper, records packed
     * up from offset 14. Guards the convention against re-inversion. */
    auto rec1 = make_record(2, u"aaaa", 2, 50, 0, 0, 0, 0, 0, 0);
    auto rec2 = make_record(2, u"bbbb", 2, 51, 0, 0, 0, 0, 0, 0);
    auto node = make_leaf_node({rec1, rec2}, 0);
    const u32 off0 = rd16(node.data() + NODE_SIZE - 2);
    const u32 off1 = rd16(node.data() + NODE_SIZE - 4);
    const u32 free = rd16(node.data() + NODE_SIZE - 6);
    EXPECT_EQ(14u, off0) << "record 0 always starts at 14 (TN1150)";
    EXPECT_EQ(14u + rec1.size(), off1) << "record 1 follows record 0";
    EXPECT_EQ(14u + rec1.size() + rec2.size(), free) << "deepest slot = free start";
    EXPECT_GT(off1, off0);
    EXPECT_GT(free, off1);
    EXPECT_EQ(0, memcmp(node.data() + 14, rec1.data(), rec1.size()));
    EXPECT_EQ(0, memcmp(node.data() + 14 + rec1.size(), rec2.data(), rec2.size()));
}

/* ---- N-028 / N-038 / N-005 (diff-to-fix minors session) ----------------- */

/* patch a file record's ownerFlags (+0x29) and FinderInfo fileType (+0x30)
 * inside a make_record() blob (key is padded to even, then the body) */
void patch_file_record(cr::u8* rec, const std::u16string& name, cr::u8 owner_flags,
                       const char file_type[4]) {
    const cr::u32 key_len = cr::u32(2 + 4 + 2 + name.size() * 2);
    cr::u32 body_off = key_len;
    if (body_off & 1)
        ++body_off;
    rec[body_off + 0x29] = owner_flags;
    if (file_type)
        std::memcpy(rec + body_off + 0x30, file_type, 4);
}

TEST(HfsPlus, DotfileAndPrivateDataFiltered) {
    /* N-028a: the og's result sink (FUN_1800057d0 :3714-3718) destroys
     * dot-prefixed names and "HFS+ Private Data" before emission - the
     * core's scan is the equivalent sink and must filter both. */
    auto d = build_volume();
    auto dot_rec = make_record(2, u".DS_Store", 2, 40, 12, B_GONE_DATA, 1, 0, 0, 0);
    auto priv_rec = make_record(2, u"HFS+ Private Data", 1, 41, 0, 0, 0, 0, 0, 0);
    auto keep_rec = make_record(2, u"visible.txt", 2, 42, 12, B_GONE_DATA, 1, 0, 0, 0);
    /* new LIVE leaf at block 9 (in-use per the bitmap) - the freed leaf2
     * stays the orphan-sweep source, untouched */
    constexpr u32 B_CAT_LEAF3 = 9; /* block 9 = catalog node 7 */
    constexpr u32 NODE3 = 7;       /* node index (catalog fork base = block 2) */
    auto leaf3 = make_leaf_node({dot_rec, priv_rec, keep_rec}, 0);
    memcpy(d.data() + u64(B_CAT_LEAF3) * BS, leaf3.data(), NODE_SIZE);
    {
        u8* vh = d.data() + 1024;
        write_fork(vh + 0x110, 8 * NODE_SIZE, B_CAT_HDR, 8); /* catalog span */
        u8* hdr = d.data() + u64(B_CAT_HDR) * BS;
        be32(hdr + 14 + 0x06, 7);     /* leafRecords: +3 */
        be32(hdr + 14 + 0x0E, NODE3); /* lastLeafNode */
        be32(hdr + 14 + 0x16, 8);     /* totalNodes */
        be32(hdr + 14 + 0x1A, 5);     /* freeNodes: nodes 2..6 */
        u8* leaf1 = d.data() + u64(B_CAT_LEAF1) * BS;
        be32(leaf1 + 0, NODE3); /* chain: leaf1 -> leaf3 */
    }
    MemDevice dev(d);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    HfsScanFlags fl;
    HfsScanStats st;
    ASSERT_EQ(scan_hfsplus(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st), OK);
    int dot = 0, priv = 0, keep = 0;
    std::function<void(tree::FileNode*)> scan = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == ".DS_Store")
                ++dot;
            if (c->name_utf8() == "HFS+ Private Data")
                ++priv;
            if (c->name_utf8() == "visible.txt")
                ++keep;
            scan(c);
        }
    };
    scan(ra);
    scan(rd);
    scan(rl);
    EXPECT_EQ(dot, 0) << "dotfiles must be filtered (og sink parity)";
    EXPECT_EQ(priv, 0) << "the hardlink metadata dir must be filtered";
    EXPECT_EQ(keep, 1) << "regular files are unaffected";
    EXPECT_EQ(st.sink_filtered, 2u);
}

TEST(HfsPlus, PrdfRecordFolderized) {
    /* N-028c: a FILE record whose FinderInfo fileType is 'prdf' is
     * folderized by the og (FUN_180009940 :7687-89 - mark, folder kind,
     * forks not parsed). */
    auto d = build_volume();
    auto prdf_rec = make_record(2, u"prdfthing", 2, 43, 512, B_GONE_DATA, 1, 0, 0, 0);
    patch_file_record(prdf_rec.data(), u"prdfthing", 0, "prdf");
    constexpr u32 B_CAT_LEAF3 = 9;
    constexpr u32 NODE3 = 7;
    auto leaf3 = make_leaf_node({prdf_rec}, 0);
    memcpy(d.data() + u64(B_CAT_LEAF3) * BS, leaf3.data(), NODE_SIZE);
    {
        u8* vh = d.data() + 1024;
        write_fork(vh + 0x110, 8 * NODE_SIZE, B_CAT_HDR, 8);
        u8* hdr = d.data() + u64(B_CAT_HDR) * BS;
        be32(hdr + 14 + 0x06, 5);
        be32(hdr + 14 + 0x0E, NODE3);
        be32(hdr + 14 + 0x16, 8);
        be32(hdr + 14 + 0x1A, 5);
        u8* leaf1 = d.data() + u64(B_CAT_LEAF1) * BS;
        be32(leaf1 + 0, NODE3);
    }
    MemDevice dev(d);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    HfsScanFlags fl;
    HfsScanStats st;
    ASSERT_EQ(scan_hfsplus(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st), OK);
    tree::FileNode* n = nullptr;
    std::function<void(tree::FileNode*)> scan = [&](tree::FileNode* p) {
        for (tree::FileNode* c = p->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "prdfthing")
                n = c;
            scan(c);
        }
    };
    scan(ra);
    scan(rd);
    scan(rl);
    ASSERT_NE(n, nullptr);
    EXPECT_NE(n->props()->attributes & 0x10, 0u) << "'prdf' must emit as a folder";
    EXPECT_EQ(n->props()->logical_size, 0u) << "forks are not parsed for prdf records";
    EXPECT_TRUE(n->props()->extents.empty()) << "no content extents for prdf records";
    EXPECT_EQ(st.prf_folderized, 1u);
}

TEST(HfsPlus, RecordBit29SelectsResourceFork) {
    /* N-038: record ownerFlags (+0x29) bit 0x20 selects the RESOURCE
     * fork as the node's content (FUN_180009940 :7665/:7677-78). A marked
     * file with BOTH forks populated must emit the rsrc bytes, not the
     * data fork. */
    auto d = build_volume();
    /* both forks populated, marked: rsrc at B_RSRC_DATA ("notes resource
     * fork data"), data at B_FILE_DATA ("readme content here!!") */
    auto marked_rec = make_record(2, u"marked.rsrc", 2, 44, 22, B_FILE_DATA, 1, 25, B_RSRC_DATA, 1);
    patch_file_record(marked_rec.data(), u"marked.rsrc", 0x20, nullptr);
    constexpr u32 B_CAT_LEAF3 = 9;
    constexpr u32 NODE3 = 7;
    auto leaf3 = make_leaf_node({marked_rec}, 0);
    memcpy(d.data() + u64(B_CAT_LEAF3) * BS, leaf3.data(), NODE_SIZE);
    {
        u8* vh = d.data() + 1024;
        write_fork(vh + 0x110, 8 * NODE_SIZE, B_CAT_HDR, 8);
        u8* hdr = d.data() + u64(B_CAT_HDR) * BS;
        be32(hdr + 14 + 0x06, 5);
        be32(hdr + 14 + 0x0E, NODE3);
        be32(hdr + 14 + 0x16, 8);
        be32(hdr + 14 + 0x1A, 5);
        u8* leaf1 = d.data() + u64(B_CAT_LEAF1) * BS;
        be32(leaf1 + 0, NODE3);
    }
    MemDevice dev(d);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    HfsScanFlags fl;
    HfsScanStats st;
    ASSERT_EQ(scan_hfsplus(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st), OK);
    tree::FileNode* n = nullptr;
    std::function<void(tree::FileNode*)> scan = [&](tree::FileNode* p) {
        for (tree::FileNode* c = p->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "marked.rsrc")
                n = c;
            scan(c);
        }
    };
    scan(ra);
    scan(rd);
    scan(rl);
    ASSERT_NE(n, nullptr);
    ASSERT_NE(n->props(), nullptr);
    EXPECT_EQ(n->props()->logical_size, 25u) << "the rsrc fork's size must win";
    ASSERT_EQ(n->props()->extents.size(), 1u);
    EXPECT_EQ(n->props()->extents[0].offset, u64(B_RSRC_DATA) * BS)
        << "content extents must come from the resource fork";
    EXPECT_EQ(n->props()->extents[0].length, BS) << "extent length is block-granular";
    EXPECT_EQ(st.rsrc_marked_files, 1u);
    /* the unmarked sibling (fixture's readme.txt) still takes its data fork */
    tree::FileNode* readme = nullptr;
    std::function<void(tree::FileNode*)> scan2 = [&](tree::FileNode* p) {
        for (tree::FileNode* c = p->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "readme.txt")
                readme = c;
            scan2(c);
        }
    };
    scan2(ra);
    ASSERT_NE(readme, nullptr);
    EXPECT_EQ(readme->props()->logical_size, 26u) << "unmarked files keep the data fork";
    EXPECT_EQ(readme->props()->extents[0].offset, u64(B_FILE_DATA) * BS);
}

TEST(HfsPlus, LargeBlockSizeAccepted) {
    /* N-005: the og has no upper bound beyond the 512-multiple mask; the
     * core's cap is relaxed 64 KiB -> 16 MiB with a device-size product
     * sanity at scan time. Zero stays rejected (geometrically unusable -
     * registered hardening). */
    auto d = build_volume();
    u8* vh = d.data() + 1024;
    be32(vh + 0x28, 1u << 20); /* blockSize = 1 MiB (512-multiple) */
    be32(vh + 0x2C, 15);       /* totalBlocks: 15 MiB < 16 MiB image */
    hfsplus::VolumeHeader h;
    EXPECT_TRUE(hfsplus::validate_volume_header(vh, h))
        << "1 MiB allocation blocks with valid geometry must validate";
    EXPECT_EQ(h.block_size, 1u << 20);

    auto d2 = build_volume();
    u8* vh2 = d2.data() + 1024;
    be32(vh2 + 0x28, 0); /* blockSize 0: geometrically unusable */
    be32(vh2 + 0x2C, 16);
    hfsplus::VolumeHeader h2;
    EXPECT_FALSE(hfsplus::validate_volume_header(vh2, h2))
        << "blockSize 0 stays rejected (registered floor)";

    auto d3 = build_volume();
    u8* vh3 = d3.data() + 1024;
    be32(vh3 + 0x28, 1u << 20);
    be32(vh3 + 0x2C, 0x10000); /* 64 GiB claimed on a 16 MiB image */
    hfsplus::VolumeHeader h3;
    EXPECT_TRUE(hfsplus::validate_volume_header(vh3, h3))
        << "VH validation alone cannot know the device (og parity)";
    {
        /* the scan-level product sanity must refuse the absurd geometry */
        MemDevice dev(d3);
        tree::ResultTree tree;
        auto* ra = tree.add_root(u"Found");
        HfsScanFlags fl;
        HfsScanStats st;
        const u32 rc =
            scan_hfsplus(&dev, fl, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st);
        EXPECT_NE(OK, rc) << "the product sanity must refuse absurd geometry";
    }
}

/* ------------------------------------------------------------------ */
/* A-005: extents-overflow tree fixture + test                          */
/* ------------------------------------------------------------------ */

namespace cr {
namespace hfsplus {
namespace {

/* A volume with a POPULATED extents-overflow tree (the base fixture's is
 * empty): block 9 = overflow header node, block 10 = overflow leaf with one
 * record for cnid 40 (fragmented.dat: initial extent blocks 11..18, extent
 * 9 = block 19 via the overflow record). 20 blocks total. */
std::vector<u8> build_volume_overflow() {
    auto d = build_volume();
    d.resize(20 * BS, 0);
    u8* vh = d.data() + 1024;
    be32(vh + 0x2C, 20);                        /* totalBlocks */
    write_fork(vh + 0xC0, 2 * NODE_SIZE, 9, 2); /* extents overflow fork */

    /* overflow header node (block 9): node 1 is the only leaf */
    u8* oh = d.data() + u64(9) * BS;
    be32(oh + 0, 1);         /* forwardLink -> leaf node 1 */
    oh[8] = 1;               /* kind = 1 (kBTHeaderNode) */
    oh[9] = 0;               /* height 0 */
    be16(oh + 10, 3);        /* numRecords */
    be32(oh + 14 + 0x0A, 1); /* firstLeafNode = 1 */
    be32(oh + 14 + 0x0E, 1); /* lastLeafNode = 1 */
    be16(oh + 14 + 0x12, NODE_SIZE);

    /* the overflow record: 12-byte key {keyLength u16 = 10 (DATA length -
     * the field itself makes the key 12 bytes), forkType 0, pad, fileID 40,
     * startBlock 8} + the 64-byte HFSPlusExtentRecord at key+12 (TN1150) */
    std::vector<u8> orec(12 + 64, 0);
    be16(orec.data(), 10);
    orec[2] = 0;                /* forkType: data fork */
    orec[3] = 0;                /* pad */
    be32(orec.data() + 4, 40);  /* fileID = fragmented.dat's cnid */
    be32(orec.data() + 8, 8);   /* startBlock (fork-relative) */
    be32(orec.data() + 12, 19); /* descriptor 0: allocation block 19 */
    be32(orec.data() + 16, 1);  /* ... count 1 */
    auto oleaf = make_leaf_node({orec}, 0);
    memcpy(d.data() + u64(10) * BS, oleaf.data(), NODE_SIZE);

    /* leaf 1 gains the fragmented file (8-block initial extent at 11..18) */
    auto root_rec = make_record(1, u"Macintosh HD", 1, 2, 0, 0, 0, 0, 0, 0);
    auto file_rec = make_record(2, u"readme.txt", 2, 20, 26, B_FILE_DATA, 1, 0, 0, 0);
    auto dir_rec = make_record(2, u"Documents", 1, 21, 0, 0, 0, 0, 0, 0);
    auto nested_rec = make_record(21, u"notes.rtf", 2, 22, 44, B_RSRC_DATA, 1, 8, B_GONE_DATA, 1);
    auto frag_rec = make_record(2, u"fragmented.dat", 2, 40, 9 * BS, 11, 8, 0, 0, 0);
    auto leaf1 = make_leaf_node({root_rec, file_rec, dir_rec, nested_rec, frag_rec}, 0);
    memcpy(d.data() + u64(B_CAT_LEAF1) * BS, leaf1.data(), NODE_SIZE);

    memset(d.data() + u64(19) * BS, 0x5A, BS); /* overflow-extent content */
    return d;
}

} // namespace

TEST(HfsPlus, ExtentsOverflowRecordResolvesFragmentedFork) {
    /* A-005: the overflow record's descriptors live at key+12, not key+10.
     * Pre-fix, the first parsed descriptor was assembled from the key's
     * startBlock bytes - a garbage block number whose extent the bounds
     * clamp silently DROPPED (fragmented tails vanished from recovery). */
    auto img = build_volume_overflow();
    MemDevice dev(img);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    HfsScanFlags fl;
    HfsScanStats st;
    u32 rc = scan_hfsplus(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(1u, st.overflow_extents) << "the overflow record must be walked";

    tree::FileNode* frag = nullptr;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props() && c->name_utf8() == "fragmented.dat")
                frag = c;
            collect(c);
        }
    };
    for (auto* r : tree.roots())
        collect(r);
    ASSERT_NE(frag, nullptr) << "the fragmented file must be found";
    EXPECT_EQ(u64(9) * BS, frag->props()->logical_size);
    ASSERT_EQ(2u, frag->props()->extents.size()) << "the overflow-sourced 9th extent must resolve";
    EXPECT_EQ(u64(11) * BS, frag->props()->extents[0].offset);
    EXPECT_EQ(u64(8) * BS, frag->props()->extents[0].length);
    EXPECT_EQ(u64(19) * BS, frag->props()->extents[1].offset);
    EXPECT_EQ(u64(BS), frag->props()->extents[1].length);
}

} // namespace hfsplus
} // namespace cr

/* ------------------------------------------------------------------ */
/* S-021: journal deleted-record harvest fixtures + tests               */
/* ------------------------------------------------------------------ */

constexpr u32 B_JINFO = 9;  /* JournalInfoBlock */
constexpr u32 B_JDATA = 14; /* harvested file's content (free in bitmap) */

void le16j(u8* p, u16 v) {
    p[0] = u8(v);
    p[1] = u8(v >> 8);
}
void le32j(u8* p, u32 v) {
    p[0] = u8(v);
    p[1] = u8(v >> 8);
    p[2] = u8(v >> 16);
    p[3] = u8(v >> 24);
}
void le64j(u8* p, u64 v) {
    le32j(p, u32(v));
    le32j(p + 4, u32(v >> 32));
}

/* Write the JournalInfoBlock (TN1150 BE fields) + the LE jnl_header. */
void write_journal_info(std::vector<u8>& d, u32 jinfo_block, u64 jbase, u64 jsize, u64 jstart,
                        u32 size_a, u32 size_b) {
    u8* ji = d.data() + u64(jinfo_block) * BS;
    be32(ji + 0, 1); /* flags: bit 0 set (the og's gate) */
    /* +4..+0x23: device_signature[8] = zeros (TN1150; NO magic field) */
    be64(ji + 0x24, jbase); /* journal byte offset (the og's decode -
                             * FUN_18000db70 swaps [0x24..0x2B]) */
    be64(ji + 0x2C, jsize); /* journal size ([0x2C..0x33]) */
    u8* jh = d.data() + jbase;
    le32j(jh + 0, 0x4A4E4C78); /* JNLx (jnl.c, LE) */
    le32j(jh + 4, 0x12345678); /* endian marker */
    le64j(jh + 8, jbase + jsize);
    le64j(jh + 0x10, jstart);
    le64j(jh + 0x18, jsize);  /* cross-checked against the info size */
    le32j(jh + 0x20, size_a); /* transaction block-list size */
    le32j(jh + 0x28, size_b); /* walk/classification block size */
}

/* Write one transaction block-list at ring offset `tx_off` (relative to
 * jbase). magic = (size_a >> 4) - 1 (the corpus-decoded og gate);
 * count = entries + 1 (the og loop processes count-1 entries). */
void write_tx(std::vector<u8>& d, u64 jbase, u64 tx_off, u32 size_a, u32 entries) {
    u8* tx = d.data() + jbase + tx_off;
    /* journal-internal structures are LITTLE-endian (jnl.c; the og's
     * ushort loads are native LE) */
    le16j(tx + 0, u16((size_a >> 4) - 1));
    le16j(tx + 2, u16(entries + 1));
}

/* One 16-byte entry: {u64 target_block LE, u32 image_size LE, pad}. */
void write_tx_entry(std::vector<u8>& d, u64 jbase, u64 tx_off, u32 idx, u64 target_block,
                    u32 image_size) {
    u8* e = d.data() + jbase + tx_off + 0x20 + u64(idx) * 16;
    le64j(e + 0, target_block);
    le32j(e + 8, image_size);
}

/* A 512-byte index-kind node image (passes the node gate, skipped by the
 * leaf-only rule). */
std::vector<u8> make_index_node() {
    std::vector<u8> n(512, 0);
    n[8] = 1; /* kBTIndexNode */
    n[9] = 0; /* height 0 (og FUN_180010720 gate) */
    return n;
}

/* Journal layout (test A): JournalInfoBlock @9; journal region 10..13
 * (4 blocks); tx at ring BS (block 11); entry[0] image = a catalog leaf
 * (block 12) carrying {journaldeleted.dat, readme.txt-live-copy};
 * entry[1] image = an index node (block 13, 512B) - leaf gate skips it. */
static std::vector<u8> build_volume_journal() {
    auto d = build_volume();
    d.resize(16 * BS, 0);                  /* already 16 blocks */
    be32(d.data() + 1024 + 0x04, 0x2000);  /* kHFSVolumeJournaledBit */
    be32(d.data() + 1024 + 0x0C, B_JINFO); /* VH journalInfoBlock */

    const u64 jbase = u64(10) * BS;
    const u64 jsize = u64(4) * BS;
    write_journal_info(d, B_JINFO, jbase, jsize, BS /*start*/, BS /*size_a*/, BS /*size_b*/);
    write_tx(d, jbase, BS, BS, 2);

    /* entry[0]: catalog leaf with the harvested records */
    auto jd = make_record(2, u"journaldeleted.dat", 2, 40, 512, B_JDATA, 1, 0, 0, 0);
    auto live_copy = make_record(2, u"readme.txt", 2, 20, 26, B_FILE_DATA, 1, 0, 0, 0);
    auto leaf = make_leaf_node({jd, live_copy}, 0);
    memcpy(d.data() + jbase + u64(2) * BS, leaf.data(), NODE_SIZE);
    write_tx_entry(d, jbase, BS, 0, B_CAT_LEAF2 /*catalog fork*/, NODE_SIZE);

    /* entry[1]: index-kind image - read + gated, but not a leaf */
    auto idx = make_index_node();
    memcpy(d.data() + jbase + u64(3) * BS, idx.data(), 512);
    write_tx_entry(d, jbase, BS, 1, B_CAT_LEAF2, 512);

    /* bitmap: 8..13 in use (journal region live), 14..15 free */
    d[u64(5) * BS + 1] = 0xFC;
    return d;
}

TEST(HfsPlus, JournalHarvestEmitsDeletedRecords) {
    auto img = build_volume_journal();
    MemDevice dev(img);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    HfsScanFlags fl;
    HfsScanStats st;
    u32 rc = scan_hfsplus(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);

    /* the ring walk: one transaction, two images, one leaf */
    EXPECT_EQ(1u, st.journal_tx);
    EXPECT_EQ(2u, st.journal_images);
    EXPECT_EQ(1u, st.journal_leaves);
    EXPECT_EQ(1u, st.journal_records);
    EXPECT_EQ(1u, st.journal_emitted);

    /* the harvested file surfaces in the deleted tier with its content
     * extents (data block 14 free -> Excellent) */
    tree::FileNode* jd = nullptr;
    int readme_count = 0;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (!c->props())
                continue;
            if (c->name_utf8() == "journaldeleted.dat")
                jd = c;
            if (c->name_utf8() == "readme.txt")
                ++readme_count;
            collect(c);
        }
    };
    for (auto* r : tree.roots())
        collect(r);
    ASSERT_NE(jd, nullptr) << "the journal harvest must surface the deleted record";
    EXPECT_EQ(tree::DeletedState::Deleted, jd->deleted_state());
    EXPECT_EQ(u64(512), jd->props()->logical_size);
    ASSERT_EQ(1u, jd->props()->extents.size());
    EXPECT_EQ(u64(B_JDATA) * BS, jd->props()->extents[0].offset);
    EXPECT_EQ(tree::Confidence::Excellent, jd->confidence());
    /* differential: the live-copy record in the same image is suppressed
     * (live catalog holds the same key with the same fileID) */
    EXPECT_EQ(1, readme_count) << "journal copy of a live record must not double-emit";
}

TEST(HfsPlus, JournalDifferentialDifferentFileIdKept) {
    /* A harvested record whose (parent, name) matches a LIVE record but
     * with a DIFFERENT fileID is kept (og :13958-62 - the file was
     * re-created: the old incarnation is deleted evidence). */
    auto img = build_volume_journal();
    /* overwrite the image's second record: same key as live readme.txt,
     * cnid 21 (a stale incarnation; live readme.txt is cnid 20) */
    auto stale = make_record(2, u"readme.txt", 2, 21, 26, B_FILE_DATA, 1, 0, 0, 0);
    auto jd = make_record(2, u"journaldeleted.dat", 2, 40, 512, B_JDATA, 1, 0, 0, 0);
    auto leaf = make_leaf_node({jd, stale}, 0);
    memcpy(img.data() + u64(10) * BS + u64(2) * BS, leaf.data(), NODE_SIZE);

    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    HfsScanFlags fl;
    HfsScanStats st;
    u32 rc = scan_hfsplus(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(2u, st.journal_emitted);
    int deleted_readme = 0;
    for (tree::FileNode* c = rd->first_child(); c; c = c->next_sibling()) {
        if (c->props() && c->name_utf8() == "readme.txt")
            ++deleted_readme;
    }
    EXPECT_EQ(1, deleted_readme) << "stale incarnation (different fileID) is kept";
}

TEST(HfsPlus, JournalBadHeaderGates) {
    /* invalid JournalInfoBlock magic / jnl magic / size cross-check each
     * degrade to a no-harvest (never a scan failure) */
    for (int mode = 0; mode < 3; ++mode) {
        auto img = build_volume_journal();
        if (mode == 0)
            be32(img.data() + u64(B_JINFO) * BS + 0,
                 0); /* flags: bit 0 clear (the og's :13513 gate) */
        else if (mode == 1)
            le32j(img.data() + u64(10) * BS + 0, 0xDEADBEEF); /* jnl magic */
        else
            le64j(img.data() + u64(10) * BS + 0x18, 12345); /* size mismatch */
        MemDevice dev(img);
        tree::ResultTree tree;
        auto* ra = tree.add_root(u"Found");
        auto* rd = tree.add_root(u"Deleted");
        auto* rl = tree.add_root(u"Lost Files");
        HfsScanFlags fl;
        HfsScanStats st;
        u32 rc = scan_hfsplus(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
        ASSERT_EQ(OK, rc) << "mode " << mode << ": " << err_text(rc);
        EXPECT_EQ(0u, st.journal_tx) << "mode " << mode;
        EXPECT_EQ(0u, st.journal_emitted) << "mode " << mode;
        /* the freed-node sweep still works (deleted.bin present) */
        bool has_bin = false;
        for (tree::FileNode* c = rd->first_child(); c; c = c->next_sibling())
            if (c->props() && c->name_utf8() == "deleted.bin")
                has_bin = true;
        EXPECT_TRUE(has_bin) << "mode " << mode;
    }
}

TEST(HfsPlus, JournalWrapHandled) {
    /* image spanning the ring end: journal = 3 blocks; the transaction
     * block-list is 6144 bytes (size_a), so the image starts at ring
     * 10240 and its last 2048 bytes wrap onto ring [0, 2048) - the split
     * read must reconstruct the node (records + offset array live in the
     * wrapped tail; the jnl header's 44 bytes overlap only the node's
     * free space). */
    auto d = build_volume();
    be32(d.data() + 1024 + 0x04, 0x2000); /* kHFSVolumeJournaledBit */
    be32(d.data() + 1024 + 0x0C, B_JINFO);

    const u64 jbase = u64(10) * BS;
    const u64 jsize = u64(3) * BS;
    /* write the wrapped image FIRST (tail at ring 0), then the header
     * overwrites the node's free-space bytes at ring [0,44) */
    auto wr = make_record(2, u"wrapped.dat", 2, 50, 512, 13, 1, 0, 0, 0);
    auto leaf = make_leaf_node({wr}, 0);
    memcpy(d.data() + jbase + 10240, leaf.data(), 2048);    /* image head */
    memcpy(d.data() + jbase + 0, leaf.data() + 2048, 2048); /* image tail */
    write_journal_info(d, B_JINFO, jbase, jsize, BS, BS + 2048, BS);
    write_tx(d, jbase, BS, BS + 2048, 1);
    write_tx_entry(d, jbase, BS, 0, B_CAT_LEAF2, NODE_SIZE);

    /* bitmap: 8..12 in use (journal live), 13..15 free */
    d[u64(5) * BS + 1] = 0xF8;

    MemDevice dev(d);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    HfsScanFlags fl;
    HfsScanStats st;
    u32 rc = scan_hfsplus(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(1u, st.journal_tx);
    EXPECT_EQ(1u, st.journal_emitted);
    tree::FileNode* found = nullptr;
    for (tree::FileNode* c = rd->first_child(); c; c = c->next_sibling())
        if (c->props() && c->name_utf8() == "wrapped.dat")
            found = c;
    ASSERT_NE(found, nullptr) << "the wrap-spanning image must be reconstructed";
    EXPECT_EQ(u64(512), found->props()->logical_size);
    ASSERT_EQ(1u, found->props()->extents.size());
    EXPECT_EQ(u64(13) * BS, found->props()->extents[0].offset);
}

TEST(HfsPlus, JournalHarvestDisabledByFlag) {
    /* the explicit off switch: no journal reads, no harvest (the S-021
     * convergence kept the landed pass; this guard pins the flag) */
    auto img = build_volume_journal();
    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    HfsScanFlags fl;
    fl.journal_harvest = false;
    HfsScanStats st;
    u32 rc = scan_hfsplus(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(0u, st.journal_tx);
    EXPECT_EQ(0u, st.journal_images);
    EXPECT_EQ(0u, st.journal_emitted);
}

TEST(HfsPlus, JournalSameKeyDifferentFileIdBothKept) {
    /* The og's harvest map is ordered by {key, fileID} (the dedup compare
     * at :13974+ matches BOTH): two journal records under the same
     * (parent, name) with different fileIDs are two distinct deleted
     * incarnations - BOTH emit. (The audit's "cnid != parentCNID" reading
     * mis-parsed the compare operand: `puVar25 + 4` is ushort* arithmetic
     * = DATA+8 = fileID, so the rule is fileID-vs-fileID, and there is no
     * parent-skip. S-021 convergence correction.) */
    auto img = build_volume_journal();
    auto dup_a = make_record(2, u"dup.txt", 2, 41, 100, B_JDATA, 1, 0, 0, 0);
    auto dup_b = make_record(2, u"dup.txt", 2, 42, 200, B_JDATA, 1, 0, 0, 0);
    auto leaf = make_leaf_node({dup_a, dup_b}, 0);
    memcpy(img.data() + u64(10) * BS + u64(2) * BS, leaf.data(), NODE_SIZE);

    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    HfsScanFlags fl;
    HfsScanStats st;
    u32 rc = scan_hfsplus(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(2u, st.journal_records) << "both {key, fileID} entries survive the map";
    EXPECT_EQ(2u, st.journal_emitted);
    int dup_count = 0;
    u64 sizes = 0;
    for (tree::FileNode* c = rd->first_child(); c; c = c->next_sibling()) {
        if (c->props() && c->name_utf8() == "dup.txt") {
            ++dup_count;
            sizes += c->props()->logical_size;
        }
    }
    EXPECT_EQ(2, dup_count) << "same key, different fileID: both incarnations emit";
    EXPECT_EQ(u64(300), sizes) << "the two incarnations carry their own sizes (100 + 200)";
}

TEST(HfsPlus, JournalAttrDecmpfsFallbackFeedsEmptyForkFile) {
    /* The attributes branch (og :14085-14145, not-found-only differential)
     * feeds the decmpfs fallback for harvested files (FUN_180009f40): a
     * deleted file whose com.apple.decmpfs attribute survives only in the
     * journal resolves its content from the journal image (mode 3: the
     * zlib stream lives inside the attribute value, inside the image). */
    auto img = build_volume_journal();
    img.resize(17 * BS, 0); /* block 15 = the attributes-fork header node */

    /* harvested catalog record: ghost.pages, EMPTY data fork */
    auto ghost = make_record(2, u"ghost.pages", 2, 45, 0, 0, 0, 0, 0, 0);
    auto leaf = make_leaf_node({ghost}, 0);
    memcpy(img.data() + u64(10) * BS + u64(2) * BS, leaf.data(), NODE_SIZE);

    /* live attributes tree: a header-only node at block 15 (no records -
     * the journal attribute survives the not-found-only differential) */
    u8* vh = img.data() + 1024;
    write_fork(vh + 0x160, 1 * BS, 15, 1);
    u8* ahdr = img.data() + u64(15) * BS;
    ahdr[8] = 1; /* kind = header */
    ahdr[9] = 0;
    be16(ahdr + 10, 3);
    be16(ahdr + 14 + 0x12, NODE_SIZE);
    be32(ahdr + 14 + 0x0A, 0); /* firstLeaf = 0 */
    be32(ahdr + 14 + 0x0E, 0); /* lastLeaf = 0 */

    /* entry[1] becomes an attributes-branch image: a leaf carrying the
     * com.apple.decmpfs record for cnid 45 (mode 3, zlib payload) */
    const std::string plain = "journal-only decmpfs payload for ghost.pages";
    const std::vector<u8> zbuf = zlib_compress(plain);
    std::vector<u8> value = decmpfs_value(3, u32(plain.size()), 0);
    value.insert(value.end(), zbuf.begin(), zbuf.end());
    auto attr_rec = make_attr_record(45, u"com.apple.decmpfs", 0x10, value);
    auto attr_leaf = make_leaf_node({attr_rec}, 0);
    memcpy(img.data() + u64(10) * BS + u64(3) * BS, attr_leaf.data(), NODE_SIZE);
    write_tx_entry(img, u64(10) * BS, BS, 1, 15 /*attributes fork*/, NODE_SIZE);

    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    HfsScanFlags fl;
    HfsScanStats st;
    u32 rc = scan_hfsplus(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(1u, st.journal_attr_records) << "the journal attribute record is kept";

    tree::FileNode* node = nullptr;
    for (tree::FileNode* c = rd->first_child(); c; c = c->next_sibling())
        if (c->props() && c->name_utf8() == "ghost.pages")
            node = c;
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(u64(plain.size()), node->props()->logical_size)
        << "the decmpfs uncompressed size wins over the empty fork";
    ASSERT_EQ(1u, node->props()->extents.size());
    EXPECT_NE(0u, node->props()->extents[0].codec) << "the content extent is codec-tagged (kZlib)";
    EXPECT_EQ(u64(zbuf.size()), node->props()->extents[0].psize)
        << "the on-disk payload size is the zlib stream inside the attribute value";
    EXPECT_EQ(1u, st.decmpfs_files);
}

/* ------------------------------------------------------------------ */
/* S-003/S-021 verification-pass tests (closing session, df5 lineage): */
/* the og's entry gates, the asm-exact validator strictness, and the   */
/* header-invalid fallback salvage ladder.                             */
/* ------------------------------------------------------------------ */

TEST(HfsPlus, JournalRequiresVolumeJournaledBit) {
    /* og :13456 - kHFSVolumeJournaledBit (0x2000) gates the whole journal
     * path; a stale nonzero journalInfoBlock without the bit must not
     * read the journal at all (TN1150: the field is only valid with the
     * bit set). */
    auto img = build_volume_journal();
    be32(img.data() + 1024 + 0x04, 0); /* clear the journaled bit */
    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    HfsScanFlags fl;
    HfsScanStats st;
    u32 rc = scan_hfsplus(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(0u, st.journal_tx) << "the journaled bit must gate the harvest";
    EXPECT_EQ(0u, st.journal_emitted);
}

TEST(HfsPlus, JournalInfoBlockHasNoMagicField) {
    /* TN1150-verbatim: device_signature[8] occupies +4..+0x23 (zeros on
     * internal journals) - the harvest must run WITHOUT any magic there.
     * The pre-correction code required 'jnlx'@+4 and silently harvested
     * nothing on every real journaled volume. */
    auto img = build_volume_journal();
    /* poison the whole device_signature region: real volumes carry zeros,
     * but ANY value must be ignored - the og swaps only [0..3]/+0x24/+0x2C */
    memset(img.data() + u64(B_JINFO) * BS + 4, 0xAB, 0x20);
    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    HfsScanFlags fl;
    HfsScanStats st;
    u32 rc = scan_hfsplus(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(1u, st.journal_tx) << "device_signature bytes must be ignored";
    EXPECT_EQ(1u, st.journal_emitted);
}

TEST(HfsPlus, JournalNodeImageRejectsInconsistentOffsetTable) {
    /* asm-exact FUN_180010720: a mid-chain offset-table violation REJECTS
     * the image (0x18001080c/:812 jump past the success flag at :825) -
     * the og never harvests from a corrupt-tail node. The lenient break
     * accepted record 0 of such nodes. */
    auto img = build_volume_journal();
    auto jd = make_record(2, u"journaldeleted.dat", 2, 40, 512, B_JDATA, 1, 0, 0, 0);
    auto stale = make_record(2, u"readme.txt", 2, 20, 26, B_FILE_DATA, 1, 0, 0, 0);
    auto leaf = make_leaf_node({jd, stale}, 0);
    /* the offset array tail: [slot1]=14, [slot2]=rec1_off, [slot3]=free;
     * collapse slot3 to 14 (a violation: 14 <= rec1_off) */
    be16(leaf.data() + NODE_SIZE - 6, 14);
    memcpy(img.data() + u64(10) * BS + u64(2) * BS, leaf.data(), NODE_SIZE);
    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    HfsScanFlags fl;
    HfsScanStats st;
    u32 rc = scan_hfsplus(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(0u, st.journal_emitted) << "a corrupt-tail node must be rejected whole (asm-strict)";
}

TEST(HfsPlus, JournalFallbackSalvageWhenHeaderInvalid) {
    /* the og's volume-open salvage ladder (:1715-1760 + FUN_180003ea0):
     * jnl_header magic corrupt but the block at jbase is node-shaped ->
     * the size ladder salvages the first validator-passing node and its
     * records reach the deleted tier through the same differential. */
    auto img = build_volume_journal();
    img.resize(24 * BS, 0);                           /* the 32 KiB scan window must fit at jbase */
    le32j(img.data() + u64(10) * BS + 0, 0xDEADBEEF); /* corrupt jnl magic */

    /* a valid catalog leaf AT the journal start (node-shaped trigger:
     * reserved==0, kind leaf, height 1) */
    auto salv = make_record(2, u"salvaged.dat", 2, 42, 512, 14, 1, 0, 0, 0);
    auto leaf = make_leaf_node({salv}, 0);
    memcpy(img.data() + u64(10) * BS, leaf.data(), NODE_SIZE);

    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    HfsScanFlags fl;
    HfsScanStats st;
    u32 rc = scan_hfsplus(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(1u, st.journal_salvage) << "the ladder must salvage the node";
    EXPECT_EQ(0u, st.journal_tx) << "no ring walk - the header is invalid";
    tree::FileNode* salvaged = nullptr;
    for (tree::FileNode* c = rd->first_child(); c; c = c->next_sibling())
        if (c->props() && c->name_utf8() == "salvaged.dat")
            salvaged = c;
    ASSERT_NE(salvaged, nullptr) << "the salvaged record must surface";
    EXPECT_EQ(u64(512), salvaged->props()->logical_size);
    ASSERT_EQ(1u, salvaged->props()->extents.size());
    EXPECT_EQ(u64(14) * BS, salvaged->props()->extents[0].offset);
}

TEST(HfsPlus, JournalFallbackRequiresNodeShapedStart) {
    /* corrupt header + a NON node-shaped block at jbase -> no salvage
     * (the og's trigger demands the BTNodeDescriptor shape). */
    auto img = build_volume_journal();
    img.resize(24 * BS, 0);
    le32j(img.data() + u64(10) * BS + 0, 0xDEADBEEF);
    memset(img.data() + u64(10) * BS, 0xE7, 0x8000); /* garbage, not node-shaped */

    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    HfsScanFlags fl;
    HfsScanStats st;
    u32 rc = scan_hfsplus(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(0u, st.journal_salvage);
    EXPECT_EQ(0u, st.journal_emitted);
}

TEST(HfsPlus, ZeroLogicalSizeFallsBackToTotalBlocks) {
    /* N-155: the og's zero-logical-size fallback (FUN_180009940
     * :7666-7671) - a data-fork record with logicalSize == 0 and
     * totalBlocks > 0 sizes as totalBlocks << 12. The og HARDCODES 4096
     * (the volume's real block size is not consulted - an og quirk; the
     * observable is the parity target, so a 4096-blocksize fixture pins
     * it exactly). Data-fork path only: the rsrc path has no fallback
     * (evidence s42-hfs-forkfield-resolution.md §2). */
    auto d = build_volume();
    /* (a) the fallback fires: 0 logical, 16 blocks -> 65536 */
    auto rec_a = make_record(2, u"zeroblocks.dat", 2, 60, 0, B_FILE_DATA, 16, 0, 0, 0);
    /* (b) genuinely empty: 0 logical, 0 blocks -> 0 (unchanged) */
    auto rec_b = make_record(2, u"empty.dat", 2, 61, 0, 0, 0, 0, 0, 0);
    /* (c) logical wins: 0x1000 logical, 1 block -> 0x1000 */
    auto rec_c = make_record(2, u"logical.dat", 2, 62, 0x1000, B_FILE_DATA, 1, 0, 0, 0);
    /* (d) rsrc-marked: rsrc 0 logical / 16 blocks -> 0 (NO fallback on
     * the rsrc path - the og's rsrc branch skips both the size store and
     * the resolver) */
    auto rec_d = make_record(2, u"rsrc-zero.rsrc", 2, 63, 0, 0, 0, 0, B_RSRC_DATA, 16);
    patch_file_record(rec_d.data(), u"rsrc-zero.rsrc", 0x20, nullptr);
    constexpr u32 B_CAT_LEAF3 = 9;
    constexpr u32 NODE3 = 7;
    auto leaf3 = make_leaf_node({rec_a, rec_b, rec_c, rec_d}, 0);
    memcpy(d.data() + u64(B_CAT_LEAF3) * BS, leaf3.data(), NODE_SIZE);
    {
        u8* vh = d.data() + 1024;
        write_fork(vh + 0x110, 8 * NODE_SIZE, B_CAT_HDR, 8);
        u8* hdr = d.data() + u64(B_CAT_HDR) * BS;
        be32(hdr + 14 + 0x06, 5);
        be32(hdr + 14 + 0x0E, NODE3);
        be32(hdr + 14 + 0x16, 8);
        be32(hdr + 14 + 0x1A, 5);
        u8* leaf1 = d.data() + u64(B_CAT_LEAF1) * BS;
        be32(leaf1 + 0, NODE3);
    }
    MemDevice dev(d);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    HfsScanFlags fl;
    HfsScanStats st;
    ASSERT_EQ(scan_hfsplus(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st), OK);
    tree::FileNode *a = nullptr, *b = nullptr, *c = nullptr, *dd = nullptr;
    std::function<void(tree::FileNode*)> scan = [&](tree::FileNode* p) {
        for (tree::FileNode* ch = p->first_child(); ch; ch = ch->next_sibling()) {
            const std::string nm = ch->name_utf8();
            if (nm == "zeroblocks.dat")
                a = ch;
            else if (nm == "empty.dat")
                b = ch;
            else if (nm == "logical.dat")
                c = ch;
            else if (nm == "rsrc-zero.rsrc")
                dd = ch;
            scan(ch);
        }
    };
    scan(ra);
    scan(rd);
    scan(rl);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    ASSERT_NE(c, nullptr);
    ASSERT_NE(dd, nullptr);
    EXPECT_EQ(a->props()->logical_size, u64(16) << 12) << "(a) totalBlocks x 4096 fallback";
    EXPECT_EQ(b->props()->logical_size, 0u) << "(b) zero/zero stays zero (empty file)";
    EXPECT_EQ(c->props()->logical_size, 0x1000u) << "(c) a nonzero logicalSize always wins";
    EXPECT_EQ(dd->props()->logical_size, 0u) << "(d) the rsrc path has NO fallback";
}

} // namespace
} // namespace hfsplus
} // namespace cr
