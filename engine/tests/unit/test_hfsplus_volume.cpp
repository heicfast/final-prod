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

/* Assemble a leaf node from records. */
std::vector<u8> make_leaf_node(const std::vector<std::vector<u8>>& recs, u32 forward) {
    std::vector<u8> node(NODE_SIZE, 0);
    be32(node.data() + 0, forward); /* forwardLink */
    be32(node.data() + 4, 0);       /* backwardLink */
    node[8] = 0xFF;                 /* kind = -1 (kBTLeafNode, TN1150) */
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

} // namespace
} // namespace hfsplus
} // namespace cr

namespace cr {
namespace hfsplus {

/* ---- N-028 / N-038 / N-005 (diff-to-fix minors session) ----------------- */

namespace {

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

} // namespace

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

} // namespace hfsplus
} // namespace cr

/* ------------------------------------------------------------------ */
/* S-021: journal deleted-record harvest fixtures + tests               */
/* ------------------------------------------------------------------ */

namespace cr {
namespace hfsplus {
namespace {

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
    be32(ji + 0, 0);          /* flags */
    be32(ji + 4, 0x6A6E6C78); /* 'jnlx' */
    be64(ji + 8, jbase);      /* journal byte offset */
    be64(ji + 16, jsize);     /* journal size */
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

} // namespace

/* Journal layout (test A): JournalInfoBlock @9; journal region 10..13
 * (4 blocks); tx at ring BS (block 11); entry[0] image = a catalog leaf
 * (block 12) carrying {journaldeleted.dat, readme.txt-live-copy};
 * entry[1] image = an index node (block 13, 512B) - leaf gate skips it. */
static std::vector<u8> build_volume_journal() {
    auto d = build_volume();
    d.resize(16 * BS, 0);                  /* already 16 blocks */
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
            be32(img.data() + u64(B_JINFO) * BS + 4, 0xDEADBEEF); /* info magic */
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

} // namespace hfsplus
} // namespace cr
