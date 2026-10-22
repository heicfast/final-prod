/* test_hfsplus_volume.cpp - synthetic HFS+ volume end-to-end.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/hfsplus/cr_hfsplus_engine.h"
#include <gtest/gtest.h>
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

/* Assemble a leaf node from records. */
std::vector<u8> make_leaf_node(const std::vector<std::vector<u8>>& recs, u32 forward) {
    std::vector<u8> node(NODE_SIZE, 0);
    be32(node.data() + 0, forward); /* forwardLink */
    be32(node.data() + 4, 0);       /* backwardLink */
    node[8] = 0;                    /* kind = leaf */
    node[9] = 1;                    /* height */
    be16(node.data() + 10, u16(recs.size()));
    /* records packed downward, stopping below the offset array
     * (the last (nrec+1) u16 slots belong to the array) */
    u32 end = NODE_SIZE - u32(recs.size() + 1) * 2;
    std::vector<u32> offsets;
    for (const auto& r : recs) {
        end -= u32(r.size());
        memcpy(node.data() + end, r.data(), r.size());
        offsets.push_back(end);
    }
    /* offset array: entries for each record (in order), node-space BE u16,
     * occupying the final (nrec+1) u16s: offsets[0] at node_end-2*(nrec+1)+2...
     * spec: record offsets stored in reverse at the node end: the FIRST
     * record's offset is closest to the end */
    /* TN1150: record i's offset sits at nodeSize - 2*(i+1); a final
     * free-space offset follows at nodeSize - 2*(numRecords+1) */
    for (u32 i = 0; i < offsets.size(); ++i)
        be16(node.data() + NODE_SIZE - (i + 1) * 2, u16(offsets[i]));
    be16(node.data() + NODE_SIZE - (offsets.size() + 1) * 2, u16(offsets[0]));
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

    /* catalog header node (node 0): BTHeaderRec at +14 */
    u8* hdr = d.data() + u64(B_CAT_HDR) * BS;
    be32(hdr + 0, B_CAT_LEAF1);       /* forwardLink -> leaf 1 (node idx 1) */
    hdr[8] = 2;                       /* kind = header */
    be16(hdr + 10, 3);                /* numRecords */
    be16(hdr + 14 + 0x00, 1);         /* treeDepth */
    be32(hdr + 14 + 0x02, 1);         /* rootNode = leaf 1 */
    be32(hdr + 14 + 0x04, 4);         /* leafRecords */
    be32(hdr + 14 + 0x08, 1);         /* firstLeafNode = node 1 */
    be32(hdr + 14 + 0x0C, 1);         /* lastLeafNode = node 1 */
    be16(hdr + 14 + 0x10, NODE_SIZE); /* nodeSize */
    be32(hdr + 14 + 0x14, 3);         /* totalNodes */
    be32(hdr + 14 + 0x18, 1);         /* freeNodes (node 2 freed) */

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

    /* fork block-count clamp: allocation fork > total blocks */
    write_fork(vh + 0x70, 999 * BS, 0, 200);
    EXPECT_FALSE(validate_volume_header(vh, h));
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

} // namespace
} // namespace hfsplus
} // namespace cr
