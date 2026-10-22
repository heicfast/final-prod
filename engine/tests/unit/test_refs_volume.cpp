/* test_refs_volume.cpp - full ReFS engine: synthetic MSB-chain volume.
 *
 * Builds a complete ReFS 3.10 shape from the thesis/ReadFS layouts:
 * VBR -> superblock (cluster 30) -> checkpoint (clock-selected) ->
 * ObjectID table (0x600 root dir, 0x700 subdir) + parent-child + medium
 * allocator -> directory tables with dir-link/file/ID2 rows -> embedded
 * file tables ($DATA data runs). Deleted rows are graded against the
 * allocator bitmap (PartialRisk / Overwritten / MetadataOnly).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/refs/cr_refs_engine.h"
#include "cr/orchestrator/cr_scan.h"
#include "cr/tree/cr_node.h"
#include <gtest/gtest.h>
#include <cstring>
#include <vector>

namespace cr {
namespace refs {
namespace {

class RamDevice final : public IStorageDevice {
public:
    explicit RamDevice(std::vector<u8> d) : d_(std::move(d)) {
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

    std::vector<u8> d_;

private:
    Geometry g_{};
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

constexpr u32 CLUSTER = 4096;
constexpr u32 PAGE = 16384; /* 4 clusters */
constexpr u64 N_CLUSTERS = 1024;
constexpr u32 VOL_SIG = 0x11223344;

/* ---- node builder (index root + index header + data area + key index) */
struct Row {
    std::vector<u8> key;
    std::vector<u8> val;
    u16 flags = 0;
};

std::vector<u8> build_node(const std::vector<Row>& rows, const std::vector<u8>& var = {}) {
    /* index root: size u32 (= 0x28 + var), fixed size u16 = 0x28 */
    std::vector<u8> ir(0x28);
    if (!var.empty()) {
        ir.resize(0x28 + var.size());
        memcpy(ir.data() + 0x28, var.data(), var.size());
    }
    put32(ir.data() + 0x00, u32(ir.size()));
    put16(ir.data() + 0x04, 0x28);

    /* entries */
    std::vector<u8> da;
    std::vector<u32> kidx;
    u32 da_off = 0;
    for (const auto& r : rows) {
        const u32 ent_len = u32(14 + r.key.size() + r.val.size());
        std::vector<u8> ent(ent_len, 0);
        put32(ent.data(), ent_len);
        put16(ent.data() + 4, u16(14));
        put16(ent.data() + 6, u16(r.key.size()));
        put16(ent.data() + 8, r.flags);
        put16(ent.data() + 10, u16(14 + r.key.size()));
        put16(ent.data() + 12, u16(r.val.size()));
        memcpy(ent.data() + 14, r.key.data(), r.key.size());
        memcpy(ent.data() + 14 + r.key.size(), r.val.data(), r.val.size());
        kidx.push_back(0x28 + da_off); /* relative to the index header */
        da.insert(da.end(), ent.begin(), ent.end());
        da_off += ent_len;
    }

    /* index header (0x28 bytes): data area + key index bounds relative to
     * its start; root leaf node (flags 0x2) */
    std::vector<u8> ih(0x28, 0);
    put32(ih.data() + 0x00, 0x28);                                 /* data area start */
    put32(ih.data() + 0x04, 0x28 + da_off);                        /* data area end */
    put32(ih.data() + 0x08, 0);                                    /* free bytes */
    ih[0x0C] = 0;                                                  /* height */
    ih[0x0D] = 0x2;                                                /* flags: root (leaf) */
    put32(ih.data() + 0x10, 0x28 + da_off);                        /* key index start */
    put32(ih.data() + 0x14, u32(rows.size()));                     /* entry count */
    put32(ih.data() + 0x20, u32(0x28 + da_off + rows.size() * 4)); /* end */

    std::vector<u8> out;
    out.insert(out.end(), ir.begin(), ir.end());
    out.insert(out.end(), ih.begin(), ih.end());
    out.insert(out.end(), da.begin(), da.end());
    for (u32 v : kidx) {
        u8 b[4];
        put32(b, v);
        out.insert(out.end(), b, b + 4);
    }
    return out;
}

/* Page = page header (0x50) + node. */
void write_page(std::vector<u8>& img, u64 page_cluster, u32 sig, u64 table_id,
                const std::vector<u8>& node) {
    const u64 off = page_cluster * CLUSTER;
    memset(img.data() + off, 0, PAGE);
    u8* p = img.data() + off;
    put32(p + 0x00, sig);
    put32(p + 0x04, 2);
    put32(p + 0x08, 0);
    put32(p + 0x0C, VOL_SIG);
    put64(p + 0x10, 1); /* virtual allocator clock */
    put64(p + 0x18, 1); /* tree update clock */
    for (int i = 0; i < 4; ++i)
        put64(p + 0x20 + i * 8, page_cluster + i);
    put64(p + 0x40, 0);        /* table id high */
    put64(p + 0x48, table_id); /* table id low */
    memcpy(p + 0x50, node.data(), node.size());
}

std::vector<u8> u16name(const char16_t* s) {
    std::vector<u8> out;
    for (const char16_t* p = s; *p; ++p) {
        out.push_back(u8(*p & 0xFF));
        out.push_back(u8(*p >> 8));
    }
    return out;
}

/* File row value: embedded file-table root node (general info + $DATA). */
std::vector<u8> build_file_node(u64 create, u64 modify, u32 flags, u64 size, u64 alloc,
                                u64 data_lcn, u32 data_clusters, bool with_data) {
    std::vector<u8> var(0x40, 0);
    put64(var.data() + 0x00, create);
    put64(var.data() + 0x08, modify);
    put64(var.data() + 0x10, modify); /* metadata modification */
    put64(var.data() + 0x18, create); /* access */
    put32(var.data() + 0x20, flags);
    put64(var.data() + 0x30, size);
    put64(var.data() + 0x38, alloc);

    /* $DATA attribute row: key = attr key header, value = data-run node */
    std::vector<u8> run_node = build_node({
        Row{[] {
                std::vector<u8> k(8, 0); /* VCN 0 */
                return k;
            }(),
            [&] {
                std::vector<u8> v(0x18, 0);
                put64(v.data() + 0x00, data_lcn);
                put16(v.data() + 0x08, with_data ? 0x0010 : 0x0000);
                put32(v.data() + 0x14, data_clusters);
                return v;
            }()},
    });

    std::vector<u8> attr_key(12, 0);
    put32(attr_key.data() + 0x00, u32(12 + 0)); /* total length */
    put32(attr_key.data() + 0x04, 0);           /* offset in attribute */
    put16(attr_key.data() + 0x08, 0x80);        /* $DATA */

    return build_node({Row{attr_key, run_node}}, var);
}

/* Directory-link row value (thesis 4.28). */
std::vector<u8> build_dirlink_value(u64 child_tid, u64 create, u64 modify) {
    std::vector<u8> v(0x48, 0);
    put64(v.data() + 0x08, child_tid);
    put64(v.data() + 0x10, create);
    put64(v.data() + 0x18, modify);
    put64(v.data() + 0x20, modify);
    put64(v.data() + 0x28, create);
    put64(v.data() + 0x38, 0);                 /* actual size */
    put32(v.data() + 0x40, 0x10000000 | 0x10); /* flags: directory */
    return v;
}

std::vector<u8> build_volume() {
    std::vector<u8> img(N_CLUSTERS * CLUSTER, 0);
    const u64 T_CREATE = 0x01D0000000000000ull;
    const u64 T_MODIFY = 0x01D1000000000000ull;

    /* ---- VBR ---- */
    {
        u8* b = img.data();
        b[0] = 0xEB;
        b[1] = 0x76;
        b[2] = 0x90;
        memcpy(b + 3, "ReFS    ", 8);
        memcpy(b + 0x10, "FSRS", 4);
        put16(b + 0x14, 512);                   /* VBR size */
        put16(b + 0x16, 0);                     /* checksum (not validated by the engine) */
        put64(b + 0x18, N_CLUSTERS * 8);        /* sector count */
        put32(b + 0x20, 512);                   /* bytes per sector */
        put32(b + 0x24, 8);                     /* sectors per cluster */
        b[0x28] = 3;                            /* major */
        b[0x29] = 10;                           /* minor */
        put64(b + 0x38, 0xDAB891EFB891CA81ull); /* serial */
        put64(b + 0x40, 64ull << 20);           /* bytes per container */
        b[510] = 0x55;
        b[511] = 0xAA;
    }

    /* ---- superblock at cluster 30 ---- */
    {
        u8* p = img.data() + 30 * CLUSTER;
        memset(p, 0, PAGE);
        put32(p + 0x00, 0x42505553); /* "SUPB" */
        put32(p + 0x04, 2);
        put32(p + 0x08, 0);
        put32(p + 0x0C, VOL_SIG);
        put64(p + 0x10, 1);
        put64(p + 0x18, 1);
        for (int i = 0; i < 4; ++i)
            put64(p + 0x20 + i * 8, 30 + i);
        put64(p + 0x40, 0);
        put64(p + 0x48, 0);
        /* GUID: XOR of the 4 words = VOL_SIG */
        put32(p + 0x50, 0x10000000);
        put32(p + 0x54, 0x01220000);
        put32(p + 0x58, 0x00003300);
        put32(p + 0x5C, 0x00000044);
        put64(p + 0x68, 1);    /* version */
        put32(p + 0x70, 0xC0); /* checkpoint refs offset */
        put32(p + 0x74, 2);
        put32(p + 0x78, 0xD0); /* self-descriptor */
        put32(p + 0x7C, 0x68);
        put64(p + 0xC0, 100); /* checkpoint 1 cluster */
        put64(p + 0xC8, 100); /* checkpoint 2 cluster (same) */
    }

    /* ---- checkpoint at cluster 100 ---- */
    {
        u8* p = img.data() + 100 * CLUSTER;
        memset(p, 0, PAGE);
        put32(p + 0x00, 0x504B4843); /* "CHKP" */
        put32(p + 0x04, 2);
        put32(p + 0x08, 0);
        put32(p + 0x0C, VOL_SIG);
        put64(p + 0x10, 1);
        put64(p + 0x18, 1);
        for (int i = 0; i < 4; ++i)
            put64(p + 0x20 + i * 8, 100 + i);
        put64(p + 0x40, 0);
        put64(p + 0x48, 0);
        put16(p + 0x54, 3);     /* major */
        put16(p + 0x56, 10);    /* minor */
        put32(p + 0x58, 0x280); /* self-descriptor offset */
        put32(p + 0x5C, 0x68);
        put64(p + 0x60, 5);  /* checkpoint virtual clock */
        put32(p + 0x90, 13); /* pointer count */
        /* pointers: [0] ObjectID @0x138, [1] MediumAllocator @0x1A0,
         * [4] ParentChild @0x208 */
        u32 ptrs[13] = {};
        ptrs[0] = 0x138;
        ptrs[1] = 0x1A0;
        ptrs[4] = 0x208;
        for (int i = 0; i < 13; ++i)
            put32(p + 0x94 + i * 4, ptrs[i]);
        /* page references at each pointer */
        for (int i = 0; i < 4; ++i)
            put64(p + 0x138 + i * 8, 120 + i); /* ObjectID root */
        for (int i = 0; i < 4; ++i)
            put64(p + 0x1A0 + i * 8, 90 + i); /* allocator root */
        for (int i = 0; i < 4; ++i)
            put64(p + 0x208 + i * 8, 140 + i); /* parent-child root */
    }

    /* ---- medium allocator at cluster 90 (bitmap over all clusters) ---- */
    {
        /* one row: first=0, count=1024, sparse=0, bitmap 128 bytes */
        std::vector<u8> bm(128, 0);
        auto mark = [&bm](u64 c) { bm[c / 8] |= u8(1 << (c % 8)); };
        for (u64 c = 30; c < 34; ++c)
            mark(c); /* superblock */
        for (u64 c = 100; c < 104; ++c)
            mark(c); /* checkpoint */
        for (u64 c = 120; c < 124; ++c)
            mark(c);
        for (u64 c = 140; c < 144; ++c)
            mark(c);
        for (u64 c = 160; c < 164; ++c)
            mark(c);
        for (u64 c = 180; c < 184; ++c)
            mark(c);
        mark(200); /* hello.txt data (also used by over.bin: reused) */
        mark(220); /* over.bin data: allocated -> Overwritten */
        /* 210 (gone.bin) and 230 (inner.dat) stay FREE */

        std::vector<u8> val(0x18 + 128, 0);
        put64(val.data() + 0x00, 0);
        put64(val.data() + 0x08, 1024);
        put16(val.data() + 0x10, 0); /* free count */
        put16(val.data() + 0x12, 0); /* flags: not sparse */
        memcpy(val.data() + 0x18, bm.data(), 128);
        std::vector<u8> key(16, 0);
        put64(key.data(), 0);
        write_page(img, 90, 0x2B42534D /* "MSB+" */, 33, build_node({Row{key, val}}));
    }

    /* ---- ObjectID table at cluster 120 ---- */
    {
        std::vector<u8> r600_val(0x50, 0), r700_val(0x50, 0);
        for (int i = 0; i < 4; ++i) {
            put64(r600_val.data() + 0x30 + i * 8, 160 + i);
            put64(r700_val.data() + 0x30 + i * 8, 180 + i);
        }
        std::vector<u8> k600(16, 0), k700(16, 0);
        put64(k600.data(), 0x600);
        put64(k700.data(), 0x700);
        write_page(img, 120, 0x2B42534D, 1, build_node({Row{k600, r600_val}, Row{k700, r700_val}}));
    }

    /* ---- parent-child table at cluster 140 ---- */
    {
        std::vector<u8> key(16, 0), val(16, 0);
        put64(key.data(), 0x600);
        put64(key.data() + 8, 0x700);
        put64(val.data(), 0x600);
        put64(val.data() + 8, 0x700);
        write_page(img, 140, 0x2B42534D, 3, build_node({Row{key, val}}));
    }

    /* ---- root directory table 0x600 at cluster 160 ---- */
    {
        std::vector<Row> rows;

        /* directory descriptor (marker 0x10) - minimal embedded node */
        {
            std::vector<u8> key(4, 0);
            put32(key.data(), 0x10);
            rows.push_back({key, build_node({}, std::vector<u8>(0x40, 0)), 0});
        }
        /* directory link: "subdir" -> table 0x700 */
        {
            std::vector<u8> key;
            put32(std::vector<u8>(4, 0).data(), 0x00020030);
            key.resize(4);
            put32(key.data(), 0x00020030);
            std::vector<u8> nm = u16name(u"subdir");
            key.insert(key.end(), nm.begin(), nm.end());
            rows.push_back({key, build_dirlink_value(0x700, T_CREATE, T_MODIFY), 0});
        }
        /* file: "hello.txt" (active, 1 cluster @200) */
        {
            std::vector<u8> key;
            key.resize(4);
            put32(key.data(), 0x00010030);
            std::vector<u8> nm = u16name(u"hello.txt");
            key.insert(key.end(), nm.begin(), nm.end());
            rows.push_back(
                {key, build_file_node(T_CREATE, T_MODIFY, 0, 4096, 4096, 200, 1, true), 0});
        }
        /* deleted file: "gone.bin" (data @210, allocator FREE -> PartialRisk) */
        {
            std::vector<u8> key;
            key.resize(4);
            put32(key.data(), 0x00010030);
            std::vector<u8> nm = u16name(u"gone.bin");
            key.insert(key.end(), nm.begin(), nm.end());
            rows.push_back({key, build_file_node(T_CREATE, T_MODIFY, 0, 4096, 4096, 210, 1, true),
                            0x4 /* deleted */});
        }
        /* deleted file: "over.bin" (data @220, allocator SET -> Overwritten) */
        {
            std::vector<u8> key;
            key.resize(4);
            put32(key.data(), 0x00010030);
            std::vector<u8> nm = u16name(u"over.bin");
            key.insert(key.end(), nm.begin(), nm.end());
            rows.push_back({key, build_file_node(T_CREATE, T_MODIFY, 0, 4096, 4096, 220, 1, true),
                            0x4 /* deleted */});
        }
        /* deleted ID2: name evidence "moved.txt" */
        {
            std::vector<u8> key(16, 0);
            put32(key.data(), 0x80000020);
            put64(key.data() + 8, 5); /* file identifier */
            std::vector<u8> nm = u16name(u"moved.txt");
            std::vector<u8> val(0x0C + nm.size(), 0);
            put64(val.data(), 1);           /* ID2 type 1 */
            put16(val.data() + 0x08, 0x0C); /* name offset */
            put16(val.data() + 0x0A, u16(nm.size()));
            memcpy(val.data() + 0x0C, nm.data(), nm.size());
            rows.push_back({key, val, 0x4 /* deleted */});
        }
        write_page(img, 160, 0x2B42534D, 0x600, build_node(rows));
    }

    /* ---- subdir table 0x700 at cluster 180 ---- */
    {
        std::vector<Row> rows;
        std::vector<u8> key;
        key.resize(4);
        put32(key.data(), 0x00010030);
        std::vector<u8> nm = u16name(u"inner.dat");
        key.insert(key.end(), nm.begin(), nm.end());
        rows.push_back({key, build_file_node(T_CREATE, T_MODIFY, 0, 4096, 4096, 230, 1, true), 0});
        write_page(img, 180, 0x2B42534D, 0x700, build_node(rows));
    }

    /* file payload markers */
    memset(img.data() + 200 * CLUSTER, 'A', CLUSTER);
    memset(img.data() + 210 * CLUSTER, 'G', CLUSTER);
    memset(img.data() + 220 * CLUSTER, 'O', CLUSTER);
    memset(img.data() + 230 * CLUSTER, 'I', CLUSTER);
    return img;
}

void collect(tree::FileNode* n, std::vector<tree::FileNode*>& out) {
    out.push_back(n);
    for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling())
        collect(c, out);
}

TEST(Refs, BootValidation) {
    std::vector<u8> img = build_volume();
    RefsBoot b;
    EXPECT_TRUE(validate_boot(img.data(), b));
    EXPECT_EQ(3u, b.major);
    EXPECT_EQ(10u, b.minor);
    EXPECT_EQ(4096ull, b.cluster_bytes);
    EXPECT_EQ(1024ull, b.volume_clusters);
    EXPECT_TRUE(b.valid);

    /* negative: not ReFS */
    std::vector<u8> junk(512, 0x77);
    EXPECT_FALSE(validate_boot(junk.data(), b));
}

TEST(Refs, FullScanHierarchy) {
    std::vector<u8> img = build_volume();
    RamDevice dev(img);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    RefsScanFlags flags;
    RefsScanStats stats;
    u32 rc = scan_refs(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats);
    ASSERT_EQ(OK, rc) << err_text(rc);

    EXPECT_EQ(2u, stats.tables);
    EXPECT_EQ(2u, stats.files);        /* hello.txt + inner.dat */
    EXPECT_EQ(1u, stats.dirs);         /* subdir */
    EXPECT_EQ(3u, stats.deleted_rows); /* gone + over + moved (ID2) */
    EXPECT_GE(stats.data_runs, 3u);

    /* active tree: hello.txt + subdir/inner.dat */
    std::vector<tree::FileNode*> act;
    collect(ra, act);
    const tree::FileNode* hello = nullptr;
    const tree::FileNode* subdir = nullptr;
    for (const auto* n : act) {
        if (n->name_utf8() == "hello.txt")
            hello = n;
        if (n->name_utf8() == "subdir")
            subdir = n;
    }
    ASSERT_NE(nullptr, hello);
    ASSERT_NE(nullptr, subdir);

    EXPECT_EQ(4096ull, hello->props()->logical_size);
    ASSERT_EQ(1u, hello->props()->extents.size());
    EXPECT_EQ(200 * 4096ull, hello->props()->extents[0].offset);
    EXPECT_EQ(4096ull, hello->props()->extents[0].length);
    EXPECT_EQ(0x01D0000000000000ull, hello->props()->times.created);
    EXPECT_EQ(0x01D1000000000000ull, hello->props()->times.modified);
    EXPECT_EQ(u32(REFS_FS_KIND), hello->props()->fs_kind);

    /* inner.dat under subdir */
    const tree::FileNode* inner = nullptr;
    for (tree::FileNode* c = subdir->first_child(); c; c = c->next_sibling())
        if (c->name_utf8() == "inner.dat")
            inner = c;
    ASSERT_NE(nullptr, inner);
    EXPECT_EQ(1u, inner->props()->extents.size());
    EXPECT_EQ(230 * 4096ull, inner->props()->extents[0].offset);
}

TEST(Refs, DeletedGradingAndId2) {
    std::vector<u8> img = build_volume();
    RamDevice dev(img);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    RefsScanFlags flags;
    RefsScanStats stats;
    ASSERT_EQ(OK, scan_refs(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats));

    std::vector<tree::FileNode*> del;
    collect(rd, del);
    const tree::FileNode *gone = nullptr, *over = nullptr, *moved = nullptr;
    for (const auto* n : del) {
        if (n->name_utf8() == "gone.bin")
            gone = n;
        if (n->name_utf8() == "over.bin")
            over = n;
        if (n->name_utf8() == "moved.txt")
            moved = n;
    }
    ASSERT_NE(nullptr, gone);
    ASSERT_NE(nullptr, over);
    ASSERT_NE(nullptr, moved);

    EXPECT_EQ(tree::DeletedState::Deleted, gone->deleted_state());
    /* data cluster 210 is free in the allocator -> PartialRisk */
    EXPECT_EQ(tree::Confidence::PartialRisk, gone->confidence());
    /* data cluster 220 is allocated -> Overwritten */
    EXPECT_EQ(tree::Confidence::Overwritten, over->confidence());
    /* ID2 name evidence only -> MetadataOnly */
    EXPECT_EQ(tree::Confidence::MetadataOnly, moved->confidence());
}

TEST(Refs, OrchestratorProbeAndScan) {
    std::vector<u8> img = build_volume();
    RamDevice dev(img);
    EXPECT_EQ(orchestrator::FsFamily::Refs, orchestrator::ScanController::probe_family(&dev));

    orchestrator::ScanConfig cfg;
    cfg.run_deep = false;
    cfg.run_carve = false;
    orchestrator::ScanStats stats;
    tree::ResultTree tree;
    orchestrator::ScanController sc(tree, cfg);
    ASSERT_EQ(OK, sc.run(&dev, nullptr, nullptr, nullptr, stats));
    EXPECT_EQ(3u, stats.files_found); /* 2 files + 1 dir */
}

} // namespace
} // namespace refs
} // namespace cr
