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

/* File row value: embedded file-table root node (general info + $DATA).
 * run_flags/run_refsize build the data-run entry's og-grammar form
 * (N-036 tests): 0x40 + refsize 0x18 = og recognizer (block reference);
 * +0x20 marks a physical LCN (no container translation). Default
 * (0x0010, 0) keeps the historical thesis-grammar fixtures byte-identical. */
std::vector<u8> build_file_node(u64 create, u64 modify, u32 flags, u64 size, u64 alloc,
                                u64 data_lcn, u32 data_clusters, bool with_data,
                                u16 run_flags = 0x0010, u16 run_refsize = 0) {
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
                put16(v.data() + 0x08, with_data ? run_flags : u16(0x0000));
                put16(v.data() + 0x0a, run_refsize);
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

std::vector<u8> build_volume(bool container_layout = false) {
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
        if (container_layout)
            put32(b + 0x34, 8); /* N-036: container span = 8 clusters (DBR +0x34) */
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
        std::vector<Row> oid_rows;
        if (container_layout) {
            /* N-036: a system-directory container record (og
             * FUN_180002a60): 0xa0-length key, base cluster 300 @+0x90,
             * span 8 @+0x98 (== DBR +0x34). First row -> container
             * counter 0 -> map key 0. */
            std::vector<u8> ck(0xa0, 0);
            put64(ck.data() + 0x90, 300);
            put64(ck.data() + 0x98, 8);
            oid_rows.push_back({ck, std::vector<u8>(8, 0), 0});
        }
        oid_rows.push_back({k600, r600_val, 0});
        oid_rows.push_back({k700, r700_val, 0});
        write_page(img, 120, 0x2B42534D, 1, build_node(oid_rows));
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
        if (container_layout) {
            /* N-036 proof files (og run grammar: flags 0x40 + refsize 0x18):
             * "vblock.dat" - container-relative block 42 (>= span 8;
             *   (42/8 >> 1) - 2 = 0 -> container 0 @base 300 ->
             *   (42 & 7) | 300 = 302)
             * "pblock.dat" - same block with flag 0x20: physical, used
             *   as-is -> 42
             * "low.dat" - container-relative block 5 (< span): the og's
             *   below-span identity -> 5 */
            {
                std::vector<u8> key;
                key.resize(4);
                put32(key.data(), 0x00010030);
                std::vector<u8> nm = u16name(u"vblock.dat");
                key.insert(key.end(), nm.begin(), nm.end());
                rows.push_back({key, build_file_node(T_CREATE, T_MODIFY, 0, 4096, 4096, 42, 1,
                                                     true, 0x0040, 0x0018),
                                0});
            }
            {
                std::vector<u8> key;
                key.resize(4);
                put32(key.data(), 0x00010030);
                std::vector<u8> nm = u16name(u"pblock.dat");
                key.insert(key.end(), nm.begin(), nm.end());
                rows.push_back({key, build_file_node(T_CREATE, T_MODIFY, 0, 4096, 4096, 42, 1,
                                                     true, 0x0060, 0x0018),
                                0});
            }
            {
                std::vector<u8> key;
                key.resize(4);
                put32(key.data(), 0x00010030);
                std::vector<u8> nm = u16name(u"low.dat");
                key.insert(key.end(), nm.begin(), nm.end());
                rows.push_back({key, build_file_node(T_CREATE, T_MODIFY, 0, 4096, 4096, 5, 1,
                                                     true, 0x0040, 0x0018),
                                0});
            }
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

TEST(Refs, NextChainFindsNewerTable) {
    /* REFS-1 (AUDIT-LINUX-FS, HIGH): the checkpoint's reference entries
     * chain via next@ref+0x28 (page-relative offset of the NEWER reference,
     * og RefsMetadataBlockRefrenceImpl -> RefsBaseDataImplManager,
     * FUN_18000fc80/FUN_18000ff60). The checkpoint pins a possibly-older
     * ObjectID table; a newer version must win. */
    auto img = build_volume();
    const u64 T_CREATE = 0x01D0000000000000ull;
    const u64 T_MODIFY = 0x01D1000000000000ull;

    /* newer root dir table 0x600 at cluster 240: original rows + newer.txt */
    {
        std::vector<Row> rows;
        {
            std::vector<u8> key(4, 0);
            put32(key.data(), 0x10);
            rows.push_back({key, build_node({}, std::vector<u8>(0x40, 0)), 0});
        }
        /* M-027 note: the two files carry DISTINCT placements (cluster 250
         * and 270) - two live objects cannot share clusters, and the
         * object-identity keep-one would (correctly) collapse them if they
         * did; this fixture pins chain selection, not placement sharing. */
        const struct {
            const char16_t* name;
            u64 lcn;
        } files[] = {{u"hello.txt", 250}, {u"newer.txt", 270}};
        for (const auto& f : files) {
            std::vector<u8> key;
            key.resize(4);
            put32(key.data(), 0x00010030);
            std::vector<u8> nm = u16name(f.name);
            key.insert(key.end(), nm.begin(), nm.end());
            rows.push_back(
                {key, build_file_node(T_CREATE, T_MODIFY, 0, 4096, 4096, f.lcn, 1, true), 0});
        }
        write_page(img, 240, 0x2B42534D, 0x600, build_node(rows));
    }
    memset(img.data() + 250 * CLUSTER, 'N', CLUSTER);

    /* newer ObjectID table at cluster 260: 0x600 -> 240 (the newer root) */
    {
        std::vector<u8> r600_val(0x50, 0), r700_val(0x50, 0);
        for (int i = 0; i < 4; ++i) {
            put64(r600_val.data() + 0x30 + i * 8, 260 + 0); /* replaced below - same table */
            put64(r700_val.data() + 0x30 + i * 8, 180 + i);
        }
        /* 0x600 row points at the NEWER root dir (240..243) */
        for (int i = 0; i < 4; ++i)
            put64(r600_val.data() + 0x30 + i * 8, 240 + i);
        std::vector<u8> k600(16, 0), k700(16, 0);
        put64(k600.data(), 0x600);
        put64(k700.data(), 0x700);
        write_page(img, 260, 0x2B42534D, 1, build_node({Row{k600, r600_val}, Row{k700, r700_val}}));
    }

    /* chain the checkpoint's ObjectID reference (at 0x138):
     * next@0x138+0x28 = 0x160 -> newer reference at 0x2A0 (quad -> 260) */
    {
        u8* p = img.data() + 100 * CLUSTER;
        put64(p + 0x138 + 0x28, 0x2A0); /* next: page-relative offset */
        for (int i = 0; i < 4; ++i)
            put64(p + 0x2A0 + i * 8, 260 + i); /* newer ObjectID root */
        put64(p + 0x2A0 + 0x28, 0);            /* chain tail */
    }

    RamDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    RefsScanFlags flags;
    RefsScanStats stats;
    u32 rc = scan_refs(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats);
    ASSERT_EQ(OK, rc) << err_text(rc);
    std::vector<tree::FileNode*> all;
    for (auto* r : tree.roots())
        collect(r, all);
    bool newer = false, hello = false;
    for (auto* n : all) {
        if (n->props() && n->name_utf8() == "newer.txt")
            newer = true;
        if (n->props() && n->name_utf8() == "hello.txt")
            hello = true;
    }
    EXPECT_TRUE(newer) << "the chained NEWER ObjectID table must win";
    EXPECT_TRUE(hello) << "the newer root still carries the original content";
}

TEST(Refs, CheckpointLayoutMinor14) {
    /* REFS-2 (AUDIT-LINUX-FS, HIGH): minor>=14 checkpoints carry the
     * pointer list at page+0xA8 (body+0x58) instead of page+0x94
     * (body+0x44); the count stays at page+0x90 (body+0x40). og
     * FUN_180012650:13754-13758. The list exists ONLY at the new offset:
     * reading the old one yields zeros -> table_root(0) fails -> the scan
     * would ERR_BAD_MAGIC. */
    auto img = build_volume();
    {
        u8* b = img.data();
        b[0x29] = 14; /* boot minor 0x0E */
        u8* p = img.data() + 100 * CLUSTER;
        put16(p + 0x56, 14); /* checkpoint body minor */
        /* move the 13-entry u32 list from 0x94 to 0xA8 */
        u32 ptrs[13] = {};
        for (int i = 0; i < 13; ++i)
            ptrs[i] = u32(p[0x94 + i * 4]) | (u32(p[0x94 + i * 4 + 1]) << 8) |
                      (u32(p[0x94 + i * 4 + 2]) << 16) | (u32(p[0x94 + i * 4 + 3]) << 24);
        memset(p + 0x94, 0, 13 * 4);
        for (int i = 0; i < 13; ++i)
            put32(p + 0xA8 + i * 4, ptrs[i]);
    }
    RamDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    RefsScanFlags flags;
    RefsScanStats stats;
    u32 rc = scan_refs(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats);
    ASSERT_EQ(OK, rc) << err_text(rc);
    std::vector<tree::FileNode*> all;
    for (auto* r : tree.roots())
        collect(r, all);
    bool hello = false;
    for (auto* n : all)
        if (n->props() && n->name_utf8() == "hello.txt")
            hello = true;
    EXPECT_TRUE(hello) << "minor-14 checkpoint must walk through the 0xA8 list";
}

TEST(Refs, Pre34CheckpointRejected) {
    /* og version re-validation on the checkpoint body (FUN_180012650:
     * major u16@body+4 == 3 && minor u16@body+6 > 3): a 3.2 checkpoint is
     * refused exactly like the og refuses it. */
    auto img = build_volume();
    put16(img.data() + 100 * CLUSTER + 0x56, 2); /* body minor 2 */
    RamDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    RefsScanFlags flags;
    RefsScanStats stats;
    EXPECT_EQ(u32(ERR_BAD_MAGIC),
              scan_refs(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats));
}

TEST(Refs, CheckpointBodyMinorGatesLayoutNotVbr) {
    /* GAP-S6-3 (bench 40079065019: real Server-2022 ReFS 3.7 refused at
     * detection): the VBR minor and the checkpoint BODY layout minor are
     * INDEPENDENT - a 3.7 volume carries VBR minor=7 with body minor>=14.
     * The og gates the pointer-list offset on the BODY minor
     * (FUN_180012650: `lVar7 = 0x58; if (*(ushort*)(param_2 + uVar14 + 6)
     * < 0xe) lVar7 = 0x44;`); the old code keyed on boot.minor and read
     * the wrong descriptor list on every such volume -> BAD_MAGIC in 2 ms.
     * This fixture: VBR minor=7, body minor=14, list ONLY at 0xA8. */
    auto img = build_volume();
    {
        u8* b = img.data();
        b[0x29] = 7; /* VBR minor: ReFS 3.7 */
        u8* p = img.data() + 100 * CLUSTER;
        put16(p + 0x56, 14); /* checkpoint body layout minor */
        u32 ptrs[13] = {};
        for (int i = 0; i < 13; ++i)
            ptrs[i] = u32(p[0x94 + i * 4]) | (u32(p[0x94 + i * 4 + 1]) << 8) |
                      (u32(p[0x94 + i * 4 + 2]) << 16) | (u32(p[0x94 + i * 4 + 3]) << 24);
        memset(p + 0x94, 0, 13 * 4);
        for (int i = 0; i < 13; ++i)
            put32(p + 0xA8 + i * 4, ptrs[i]);
    }
    RamDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    RefsScanFlags flags;
    RefsScanStats stats;
    u32 rc = scan_refs(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats);
    ASSERT_EQ(OK, rc) << err_text(rc);
    std::vector<tree::FileNode*> all;
    for (auto* r : tree.roots())
        collect(r, all);
    bool hello = false;
    for (auto* n : all)
        if (n->props() && n->name_utf8() == "hello.txt")
            hello = true;
    EXPECT_TRUE(hello) << "body-minor-14 layout must be honored even when the VBR says 3.7";
}

TEST(Refs, VbrMinor14BodyMinorOldListStillOldLayout) {
    /* the gate is the BODY minor alone: VBR 0x0E with an old body layout
     * (minor 10) must keep the 0x94 list (the reverse independence case -
     * boot.minor must never drive the layout choice). */
    auto img = build_volume();
    {
        u8* b = img.data();
        b[0x29] = 14; /* VBR minor 0x0E */
        /* body stays minor 10 (build_volume default): list at 0x94 */
    }
    RamDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    RefsScanFlags flags;
    RefsScanStats stats;
    u32 rc = scan_refs(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats);
    ASSERT_EQ(OK, rc) << err_text(rc);
    std::vector<tree::FileNode*> all;
    for (auto* r : tree.roots())
        collect(r, all);
    bool hello = false;
    for (auto* n : all)
        if (n->props() && n->name_utf8() == "hello.txt")
            hello = true;
    EXPECT_TRUE(hello) << "old body layout (0x94) must be used when the BODY minor is old";
}

/* ---- S-020 / M-022: the og attribute layer ------------------------------ */

namespace {

/* run-row value (thesis grammar): lcn@0, flags 0x10 @8, clusters@0x14 */
std::vector<u8> run_row(u64 lcn, u32 clusters) {
    std::vector<u8> v(0x18, 0);
    put64(v.data() + 0x00, lcn);
    put16(v.data() + 0x08, 0x0010);
    put32(v.data() + 0x14, clusters);
    return v;
}

/* thesis-grammar $DATA attribute row: key {len,off,type@+8=0x80}, value =
 * a leaf node of run rows */
Row data_attr_row(const std::vector<std::pair<u64, u32>>& runs) {
    std::vector<u8> key(12, 0);
    put32(key.data() + 0x00, u32(12));
    put32(key.data() + 0x04, 0);
    put16(key.data() + 0x08, 0x80);
    std::vector<Row> rows;
    for (const auto& r : runs)
        rows.push_back({std::vector<u8>(8, 0), run_row(r.first, r.second)});
    return {key, build_node(rows)};
}

/* file-row key {0x00010030, name} */
Row file_row_key(const char16_t* name, const std::vector<u8>& node_val, u16 flags = 0) {
    std::vector<u8> key;
    key.resize(4);
    put32(key.data(), 0x00010030);
    std::vector<u8> nm = u16name(name);
    key.insert(key.end(), nm.begin(), nm.end());
    return {key, node_val, flags};
}

/* general-info variable block */
std::vector<u8> gi_block(u64 size, u64 alloc) {
    std::vector<u8> var(0x40, 0);
    put64(var.data() + 0x00, 0x01D0000000000000ull);
    put64(var.data() + 0x08, 0x01D1000000000000ull);
    put32(var.data() + 0x30, u32(size));
    put64(var.data() + 0x38, alloc);
    return var;
}

/* rewrite the root directory table (cluster 160) with the given rows */
void rewrite_root_table(std::vector<u8>& img, const std::vector<Row>& rows) {
    write_page(img, 160, 0x2B42534D, 0x600, build_node(rows));
}

/* find a 4-byte magic inside the root-table page; returns page offset */
size_t find_in_page(std::vector<u8>& img, u64 page_cluster, u32 magic) {
    const u8* base = img.data() + page_cluster * CLUSTER;
    for (size_t i = 0; i + 4 <= PAGE; ++i)
        if ((u32(base[i]) | (u32(base[i + 1]) << 8) | (u32(base[i + 2]) << 16) |
             (u32(base[i + 3]) << 24)) == magic)
            return i;
    return SIZE_MAX;
}

TEST(Refs, ResidentInlineFileRecovered) {
    /* S-020: a resident descriptor row (key {type=0x10, subtype=1}, value
     * carrying offset pair) materializes the inline content as a synthetic
     * extent - the og's CRefsResidentDataRunList (FUN_18000e490). The
     * content bytes live later in the same metadata page. */
    std::vector<u8> img = build_volume();
    {
        std::vector<u8> res_key(0x10, 0);
        put16(res_key.data() + 0x00, 0x10); /* type == value length (og gate) */
        put16(res_key.data() + 0x02, 1);    /* object subtype 1 (resident) */
        std::vector<u8> res_val(0x10, 0);
        put32(res_val.data() + 0x00, 0xC0FFEE11); /* locate-by-magic */
        /* o1 patched after the page is laid out */
        std::vector<Row> frows;
        frows.push_back({res_key, res_val});
        rewrite_root_table(img, {file_row_key(u"inline.txt", build_node(frows, gi_block(100, 0)))});
    }
    /* content at page offset 0x3F00; patch o1 = 0x3F00 - val_offset */
    {
        const size_t val_off = find_in_page(img, 160, 0xC0FFEE11);
        ASSERT_NE(val_off, SIZE_MAX);
        u8* page = img.data() + 160 * CLUSTER;
        put32(page + val_off + 8, u32(0x3F00 - val_off));
        put32(page + val_off + 0xC, 0);
        memset(page + 0x3F00, 'R', 128); /* the inline content */
    }
    RamDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    RefsScanFlags flags;
    RefsScanStats stats;
    ASSERT_EQ(OK, scan_refs(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats));
    EXPECT_GE(stats.resident_files, 1u);
    std::vector<tree::FileNode*> act;
    collect(ra, act);
    const tree::FileNode* inl = nullptr;
    for (auto* n : act)
        if (n->name_utf8() == "inline.txt")
            inl = n;
    ASSERT_NE(inl, nullptr) << "resident file emitted";
    ASSERT_EQ(inl->props()->extents.size(), 1u) << "inline content materialized";
    EXPECT_EQ(inl->props()->extents[0].offset, 160 * CLUSTER + 0x3F00);
    EXPECT_EQ(inl->props()->extents[0].length, 2 * CLUSTER); /* ceil + straddle slack */
    /* the extent covers the content bytes */
    EXPECT_EQ(img[160 * CLUSTER + 0x3F00], 'R');
}

TEST(Refs, ResidentDollarIMetadataRead) {
    /* S-020: "$I"-named type-100 resident records (the og FUN_18000ea30
     * recycle handler) are materialized with content, like the og's
     * CRefsResidentDataRunList for $Recycle.Bin metadata. */
    std::vector<u8> img = build_volume();
    {
        std::vector<u8> res_key(0x10, 0);
        put16(res_key.data() + 0x00, 100); /* type 100 == value length gate */
        std::vector<u8> res_val(100, 0);
        put32(res_val.data() + 0x00, 0xC0FFEE22);
        std::vector<Row> frows;
        frows.push_back({res_key, res_val});
        rewrite_root_table(img, {file_row_key(u"$IRECYCL", build_node(frows, gi_block(300, 0)))});
    }
    {
        const size_t val_off = find_in_page(img, 160, 0xC0FFEE22);
        ASSERT_NE(val_off, SIZE_MAX);
        u8* page = img.data() + 160 * CLUSTER;
        put32(page + val_off + 8, u32(0x3F00 - val_off));
        put32(page + val_off + 0xC, 0);
        memset(page + 0x3F00, '$', 300);
    }
    RamDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    RefsScanFlags flags;
    RefsScanStats stats;
    ASSERT_EQ(OK, scan_refs(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats));
    EXPECT_GE(stats.dollar_i_rows, 1u) << "$I recycle record read resident";
    std::vector<tree::FileNode*> act;
    collect(ra, act);
    const tree::FileNode* di = nullptr;
    for (auto* n : act)
        if (n->name_utf8() == "$IRECYCL")
            di = n;
    ASSERT_NE(di, nullptr);
    ASSERT_EQ(di->props()->extents.size(), 1u);
    EXPECT_EQ(di->props()->extents[0].offset, 160 * CLUSTER + 0x3F00);
}

TEST(Refs, MultipleDataAttributesMerged) {
    /* S-020: every data-bearing attribute row contributes (no first-match
     * break); adjacent runs coalesce (og c790/e020). */
    std::vector<u8> img = build_volume();
    {
        std::vector<Row> frows;
        frows.push_back(data_attr_row({{240, 1}}));
        frows.push_back(data_attr_row({{241, 1}})); /* adjacent -> coalesce */
        frows.push_back(data_attr_row({{250, 1}})); /* disjoint */
        rewrite_root_table(img,
                           {file_row_key(u"multi.dat", build_node(frows, gi_block(12288, 12288)))});
    }
    memset(img.data() + 240 * CLUSTER, 'M', 12 * CLUSTER);
    RamDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    RefsScanFlags flags;
    RefsScanStats stats;
    ASSERT_EQ(OK, scan_refs(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats));
    std::vector<tree::FileNode*> act;
    collect(ra, act);
    const tree::FileNode* m = nullptr;
    for (auto* n : act)
        if (n->name_utf8() == "multi.dat")
            m = n;
    ASSERT_NE(m, nullptr);
    ASSERT_EQ(m->props()->extents.size(), 2u) << "3 runs -> 2 extents (adjacent coalesced)";
    EXPECT_EQ(m->props()->extents[0].offset, 240 * CLUSTER);
    EXPECT_EQ(m->props()->extents[0].length, 2 * CLUSTER);
    EXPECT_EQ(m->props()->extents[1].offset, 250 * CLUSTER);
}

TEST(Refs, PagedRunTableFollowed) {
    /* S-020: an INNER (paged) $DATA value node is followed to its child
     * page (og FUN_18000cf30) - the old code rejected inner nodes and the
     * file lost ALL content. */
    std::vector<u8> img = build_volume();
    {
        /* child page at cluster 240: leaf node of run rows */
        std::vector<Row> runs;
        runs.push_back({std::vector<u8>(8, 0), run_row(260, 2)});
        write_page(img, 240, 0x2B42534D, 0, build_node(runs));
        memset(img.data() + 260 * CLUSTER, 'P', 2 * CLUSTER);
        /* the $DATA value = an INNER node pointing at the child page:
         * build a leaf then patch height=1 / flags=INNER (ih @node+0x28) */
        std::vector<u8> child_ref(0x20, 0);
        for (int k = 0; k < 4; ++k)
            put64(child_ref.data() + k * 8, 240 + k);
        std::vector<u8> inner = build_node({Row{std::vector<u8>(8, 0), child_ref}});
        inner[0x28 + 0x0C] = 1;   /* height */
        inner[0x28 + 0x0D] = 0x1; /* NODE_INNER */
        std::vector<Row> frows;
        std::vector<u8> key(12, 0);
        put32(key.data() + 0x00, 12);
        put16(key.data() + 0x08, 0x80);
        frows.push_back({key, inner});
        rewrite_root_table(img,
                           {file_row_key(u"paged.dat", build_node(frows, gi_block(8192, 8192)))});
    }
    RamDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    RefsScanFlags flags;
    RefsScanStats stats;
    ASSERT_EQ(OK, scan_refs(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats));
    EXPECT_GE(stats.paged_chases, 1u) << "the inner $DATA table chased its page";
    std::vector<tree::FileNode*> act;
    collect(ra, act);
    const tree::FileNode* pd = nullptr;
    for (auto* n : act)
        if (n->name_utf8() == "paged.dat")
            pd = n;
    ASSERT_NE(pd, nullptr);
    ASSERT_EQ(pd->props()->extents.size(), 1u) << "child-page runs recovered";
    EXPECT_EQ(pd->props()->extents[0].offset, 260 * CLUSTER);
    EXPECT_EQ(pd->props()->extents[0].length, 2 * CLUSTER);
}

TEST(Refs, Gen7OldGrammarDatarunAttribute) {
    /* M-022: the og's old-grammar 0x178-keyed data rows (gen < 8, this
     * fixture is minor 10 -> gen 7) yield runs. */
    std::vector<u8> img = build_volume();
    {
        std::vector<Row> frows;
        std::vector<u8> key(2, 0);
        put16(key.data(), 0x0178);
        std::vector<Row> runs;
        runs.push_back({std::vector<u8>(8, 0), run_row(260, 1)});
        frows.push_back({key, build_node(runs)});
        memset(img.data() + 260 * CLUSTER, 'G', CLUSTER);
        rewrite_root_table(img,
                           {file_row_key(u"old.dat", build_node(frows, gi_block(4096, 4096)))});
    }
    RamDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    RefsScanFlags flags;
    RefsScanStats stats;
    ASSERT_EQ(OK, scan_refs(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats));
    std::vector<tree::FileNode*> act;
    collect(ra, act);
    const tree::FileNode* o = nullptr;
    for (auto* n : act)
        if (n->name_utf8() == "old.dat")
            o = n;
    ASSERT_NE(o, nullptr);
    ASSERT_EQ(o->props()->extents.size(), 1u) << "0x178-keyed row recovered (gen < 8 grammar)";
    EXPECT_EQ(o->props()->extents[0].offset, 260 * CLUSTER);
}

TEST(Refs, RunlistGranularityMismatchDiscarded) {
    /* S-020 step 4 / N-037: runs inconsistent with the recorded allocation
     * (4 run clusters vs a 1-cluster alloc) are discarded - the og's
     * garbage-run false-positive guard (FUN_18000e020). */
    std::vector<u8> img = build_volume();
    {
        std::vector<Row> frows;
        frows.push_back(data_attr_row({{260, 1}, {261, 1}, {262, 1}, {263, 1}}));
        rewrite_root_table(img,
                           {file_row_key(u"garbage.dat", build_node(frows, gi_block(4096, 4096)))});
        memset(img.data() + 260 * CLUSTER, 'X', 4 * CLUSTER);
    }
    RamDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    RefsScanFlags flags;
    RefsScanStats stats;
    ASSERT_EQ(OK, scan_refs(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats));
    EXPECT_GE(stats.granularity_discards, 1u) << "the inconsistent runlist was discarded";
    std::vector<tree::FileNode*> act;
    collect(ra, act);
    const tree::FileNode* g = nullptr;
    for (auto* n : act)
        if (n->name_utf8() == "garbage.dat")
            g = n;
    ASSERT_NE(g, nullptr) << "the file still surfaces (metadata-only)";
    EXPECT_EQ(g->props()->extents.size(), 0u) << "no garbage extents";
}

} // namespace
TEST(Refs, StaleMsbPageDeletedFileRecovered) {
    /* M-026: a file that exists ONLY in a stale MSB+ page (an older
     * metadata-page version the live chain no longer references - the
     * normal ReFS copy-on-write outcome after deletion) is harvested by
     * the sweep as a deleted result with name/runs (og FUN_180005330). */
    std::vector<u8> img = build_volume();
    {
        /* a stale root-table-shaped page at cluster 300 (not in any live
         * table): one file row "ghost.dat" with a run at 310 */
        std::vector<Row> rows;
        std::vector<Row> frows;
        frows.push_back(data_attr_row({{310, 1}}));
        rows.push_back(file_row_key(u"ghost.dat", build_node(frows, gi_block(4096, 4096))));
        write_page(img, 300, 0x2B42534D, 0x600, build_node(rows));
        memset(img.data() + 310 * CLUSTER, 'H', CLUSTER);
    }
    RamDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    RefsScanFlags flags;
    RefsScanStats stats;
    ASSERT_EQ(OK, scan_refs(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats));
    EXPECT_GE(stats.stale_pages, 1u) << "the stale page was swept";
    std::vector<tree::FileNode*> del;
    collect(rd, del);
    const tree::FileNode* ghost = nullptr;
    for (auto* n : del)
        if (n->name_utf8() == "ghost.dat")
            ghost = n;
    ASSERT_NE(ghost, nullptr) << "stale-only file recovered as deleted";
    EXPECT_EQ(ghost->deleted_state(), tree::DeletedState::Deleted);
    ASSERT_EQ(ghost->props()->extents.size(), 1u);
    EXPECT_EQ(ghost->props()->extents[0].offset, 310 * CLUSTER);
}

TEST(Refs, StaleSweepSkipsLivePages) {
    /* M-026: pages reachable from the live chain are NOT re-harvested as
     * stale (the og's cache-skip :2910-31) - the live root table at 160
     * emits hello.txt exactly once, live. */
    std::vector<u8> img = build_volume();
    RamDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    RefsScanFlags flags;
    RefsScanStats stats;
    ASSERT_EQ(OK, scan_refs(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats));
    /* every MSB+ page of the live chain (120, 140, 160, 180, 90) is
     * live-reachable: none may appear as stale */
    EXPECT_EQ(stats.stale_pages, 0u);
    std::vector<tree::FileNode*> all;
    for (auto* r : tree.roots())
        collect(r, all);
    u32 hello_count = 0;
    for (auto* n : all)
        if (n->name_utf8() == "hello.txt")
            ++hello_count;
    EXPECT_EQ(hello_count, 1u) << "live file emitted exactly once";
}

TEST(Refs, HardLinkRowsCollapseToOneResult) {
    /* M-027: two ROW_FILE rows for one object (hard link / duplicate name
     * row) keep-one - the og's identity map resolves to a single result. */
    std::vector<u8> img = build_volume();
    {
        std::vector<Row> rows;
        /* same object: identical placement, size and times; two names */
        const std::vector<u8> node = build_node({data_attr_row({{270, 2}})}, gi_block(8192, 8192));
        rows.push_back(file_row_key(u"link_a.dat", node));
        rows.push_back(file_row_key(u"link_b.dat", node));
        rewrite_root_table(img, rows);
        memset(img.data() + 270 * CLUSTER, 'L', 2 * CLUSTER);
    }
    RamDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    RefsScanFlags flags;
    RefsScanStats stats;
    ASSERT_EQ(OK, scan_refs(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats));
    EXPECT_EQ(stats.identity_dedups, 1u) << "the second row collapsed (keep-one)";
    std::vector<tree::FileNode*> act;
    collect(ra, act);
    u32 links = 0;
    for (auto* n : act)
        if (n->name_utf8() == "link_a.dat" || n->name_utf8() == "link_b.dat")
            ++links;
    EXPECT_EQ(links, 1u) << "exactly one result for the linked object";
}

TEST(Refs, RecycleRoutedFlaggedAndDollarIRemap) {
    /* M-027: "$R…" rows route to the post-main recycle flush and the $I
     * evidence remaps the $R node to its ORIGINAL name/size/time (the
     * core-family recycle convention; the og reads the same evidence). */
    std::vector<u8> img = build_volume();
    u8* page = nullptr;
    {
        std::vector<Row> rows;
        /* $R content row: run at 270, bin name "$R7F3K2L.txt" */
        std::vector<Row> rrows;
        rrows.push_back(data_attr_row({{270, 1}}));
        rows.push_back(file_row_key(u"$R7F3K2L.txt", build_node(rrows, gi_block(4096, 4096))));
        /* $I metadata row: RESIDENT descriptor whose content (the $I
         * record: version, ORIGINAL size, deletion time, ORIGINAL path)
         * lives later in the page; patched after layout like the S-020
         * resident tests */
        std::vector<u8> ikey(0x10, 0);
        put16(ikey.data() + 0x00, 0x10);
        put16(ikey.data() + 0x02, 1);
        std::vector<u8> ival(0x10, 0);
        put32(ival.data() + 0x00, 0xC0FFEE33);
        std::vector<Row> irows;
        irows.push_back({ikey, ival});
        rows.push_back(file_row_key(u"$I7F3K2L.txt", build_node(irows, gi_block(88, 0))));
        rewrite_root_table(img, rows);
    }
    {
        const size_t val_off = find_in_page(img, 160, 0xC0FFEE33);
        ASSERT_NE(val_off, SIZE_MAX);
        page = img.data() + 160 * CLUSTER;
        put32(page + val_off + 8, u32(0x3F00 - val_off));
        put32(page + val_off + 0xC, 0);
        /* the $I record at page+0x3F00: v2, orig size 5000, time, pathlen,
         * UTF-16 path "C:\\Users\\me\\report.docx" (basename used) */
        u8* rec = page + 0x3F00;
        memset(rec, 0, 128);
        put32(rec + 0, 2);
        put64(rec + 8, 5000);
        put64(rec + 16, 0x01D2000000000000ull);
        std::vector<u8> path = u16name(u"C:\\Users\\me\\report.docx");
        put32(rec + 24, u32(path.size() / 2));
        memcpy(rec + 28, path.data(), path.size());
    }
    memset(img.data() + 270 * CLUSTER, 'W', CLUSTER);
    RamDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    RefsScanFlags flags;
    RefsScanStats stats;
    ASSERT_EQ(OK, scan_refs(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats));
    EXPECT_GE(stats.recycle_routed, 1u) << "$R routed to the recycle flush";
    std::vector<tree::FileNode*> all;
    for (auto* r : tree.roots())
        collect(r, all);
    const tree::FileNode* rr = nullptr;
    bool saw_dollar_i = false;
    for (auto* n : all) {
        if (n->name_utf8().rfind("$I7F3K2L", 0) == 0)
            saw_dollar_i = true; /* the $I row itself stays a result */
        if (n->name_utf8() == "report.docx")
            rr = n; /* the $R row AFTER the $I remap */
    }
    EXPECT_TRUE(saw_dollar_i) << "$I metadata row emitted (main map)";
    ASSERT_NE(rr, nullptr) << "$R remapped to its ORIGINAL name via $I evidence";
    EXPECT_EQ(rr->props()->logical_size, 5000u);
    EXPECT_EQ(rr->props()->times.modified, 0x01D2000000000000ull);
    EXPECT_EQ(rr->deleted_state(), tree::DeletedState::Deleted);
}

/* ===================================================================== */
/* M-031 (diff-to-fix): ReFS progress is PER-FILE (the og's quick-       */
/* searcher registers a per-file progress binder, ReFSSearchDLL          */
/* :617-619, RTTI-verified) — not one terminal tick.                     */
/* ===================================================================== */

namespace {

struct ProgressRecorder {
    std::vector<std::pair<u64, u64>> ticks;
    static bool cb(u64 done, u64 total, void* user) {
        auto* self = static_cast<ProgressRecorder*>(user);
        self->ticks.emplace_back(done, total);
        return true; /* never cancel from the progress thread */
    }
};

std::u16string numbered_name(const char16_t* base, int i) {
    std::u16string s(base);
    s.push_back(char16_t(u'0' + (i / 10) % 10));
    s.push_back(char16_t(u'0' + i % 10));
    return s;
}

} // namespace

TEST(Refs, ProgressMonotonicIntermediateTicks) {
    /* 48 files in the root table: the walk must report intermediate
     * progress every 16 emits (> 2 distinct permille values before the
     * terminal 1000 tick). Fail-before: exactly ONE terminal tick. */
    std::vector<u8> img = build_volume();
    std::vector<Row> rows;
    for (int i = 0; i < 48; ++i)
        rows.push_back(file_row_key(numbered_name(u"file", i).c_str(),
                                    build_file_node(0x01D0000000000000ull, 0x01D1000000000000ull,
                                                    0, 4096, 4096, u64(300 + i), 1, true)));
    rewrite_root_table(img, rows);
    RamDevice dev(img);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    RefsScanFlags flags;
    RefsScanStats stats;
    ProgressRecorder rec;
    u32 rc = scan_refs(&dev, flags, nullptr, ProgressRecorder::cb, &rec, tree, ra, rd, rl,
                       stats);
    ASSERT_EQ(OK, rc) << err_text(rc);

    /* intermediate ticks: (done, done+16) pairs; terminal: (n, n) */
    std::vector<u64> permilles;
    for (const auto& t : rec.ticks)
        permilles.push_back(t.second ? t.first * 1000 / t.second : 1000);
    /* monotonic */
    for (size_t k = 1; k < permilles.size(); ++k)
        EXPECT_LE(permilles[k - 1], permilles[k]) << "progress must be monotonic";
    /* the terminal tick reaches 1000 */
    ASSERT_FALSE(permilles.empty());
    EXPECT_EQ(1000u, permilles.back());
    /* more than 2 distinct intermediate values (the old engine: 1 tick) */
    const size_t distinct_intermediate =
        size_t(std::count_if(permilles.begin(), permilles.end() - 1,
                             [](u64) { return true; }));
    EXPECT_GE(rec.ticks.size(), 4u) << "48 files must produce >= 3 intermediate ticks + terminal";
    EXPECT_GE(distinct_intermediate, 3u);
}

TEST(Refs, ProgressCancellationMidWalk) {
    /* Cancel mid-walk (after the first progress tick): partial results
     * survive in the tree. */
    std::vector<u8> img = build_volume();
    std::vector<Row> rows;
    for (int i = 0; i < 48; ++i)
        rows.push_back(file_row_key(numbered_name(u"file", i).c_str(),
                                    build_file_node(0x01D0000000000000ull, 0x01D1000000000000ull,
                                                    0, 4096, 4096, u64(300 + i), 1, true)));
    rewrite_root_table(img, rows);
    RamDevice dev(img);

    struct LateCancel : CancelToken {
        std::atomic<int> ticks{0};
        bool cancelled() const override { return ticks.load() > 1; }
    } token;

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    RefsScanFlags flags;
    RefsScanStats stats;
    auto cb = +[](u64, u64, void* user) -> bool {
        auto* t = static_cast<LateCancel*>(user);
        t->ticks.fetch_add(1);
        return true;
    };
    scan_refs(&dev, flags, &token, cb, &token, tree, ra, rd, rl, stats);

    /* partial results survived: fewer than 48 files, more than none */
    size_t emitted = 0;
    std::function<void(tree::FileNode*)> count = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            ++emitted;
            count(c);
        }
    };
    count(ra);
    EXPECT_GT(emitted, 0u) << "partial results must survive cancellation";
    EXPECT_LT(emitted, 48u) << "the walk must have been cut short mid-table";
}

/* ---- N-036: container translation (og FUN_180011e90/FUN_180004820) ---- */

TEST(Refs, ContainerRelativeRunTranslatedPerOgFormula) {
    /* og grammar run (flags 0x40 + refsize 0x18, flag 0x20 CLEAR) at
     * virtual block 42, span 8, container 0 @base 300:
     * key = ((42/8 >> 1) - 2) * 8 = 0; block' = (42 & 7) | 300 = 302. */
    std::vector<u8> img = build_volume(true);
    RamDevice dev(img);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    RefsScanFlags flags;
    RefsScanStats stats;
    ASSERT_EQ(OK, scan_refs(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats))
        << err_text(OK);

    std::vector<tree::FileNode*> act;
    collect(ra, act);
    const tree::FileNode* vblock = nullptr;
    for (const auto* n : act)
        if (n->name_utf8() == "vblock.dat")
            vblock = n;
    ASSERT_NE(nullptr, vblock) << "the og-grammar (0x40) run must be recognized";
    ASSERT_EQ(1u, vblock->props()->extents.size());
    EXPECT_EQ(302 * 4096ull, vblock->props()->extents[0].offset)
        << "container-relative block 42 must translate to (42 & 7) | 300";
    EXPECT_EQ(4096ull, vblock->props()->extents[0].length);
}

TEST(Refs, PhysicalRunFlag20Untranslated) {
    /* same virtual block 42, but flag 0x20 set: physical LCN, used as-is
     * even though the container map exists (og FUN_18000c790 :8893). */
    std::vector<u8> img = build_volume(true);
    RamDevice dev(img);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    RefsScanFlags flags;
    RefsScanStats stats;
    ASSERT_EQ(OK, scan_refs(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats));

    std::vector<tree::FileNode*> act;
    collect(ra, act);
    const tree::FileNode* pblock = nullptr;
    for (const auto* n : act)
        if (n->name_utf8() == "pblock.dat")
            pblock = n;
    ASSERT_NE(nullptr, pblock);
    ASSERT_EQ(1u, pblock->props()->extents.size());
    EXPECT_EQ(42 * 4096ull, pblock->props()->extents[0].offset)
        << "flag 0x20 = physical: no container translation";
}

TEST(Refs, ContainerBlockBelowSpanStaysPhysical) {
    /* og identity rule (FUN_180011e90 :13237-13242): block < span is the
     * low physical area - never translated, map or no map. Block 5 < 8. */
    std::vector<u8> img = build_volume(true);
    RamDevice dev(img);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    RefsScanFlags flags;
    RefsScanStats stats;
    ASSERT_EQ(OK, scan_refs(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats));

    std::vector<tree::FileNode*> act;
    collect(ra, act);
    const tree::FileNode* low = nullptr;
    for (const auto* n : act)
        if (n->name_utf8() == "low.dat")
            low = n;
    ASSERT_NE(nullptr, low);
    ASSERT_EQ(1u, low->props()->extents.size());
    EXPECT_EQ(5 * 4096ull, low->props()->extents[0].offset)
        << "block < span stays physical (the og's below-span identity)";
}

TEST(Refs, SingleContainerExtentsUnchanged) {
    /* the N-036 single-container coincidence, pinned: with no container
     * records (and/or no DBR +0x34 span) the map is empty and EVERY run -
     * thesis grammar or og grammar - lands at its raw block. The base
     * fixture volumes carry no container rows, so hello.txt/inner.dat
     * extents are the pre-N-036 values. */
    std::vector<u8> img = build_volume(false);
    RamDevice dev(img);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    RefsScanFlags flags;
    RefsScanStats stats;
    ASSERT_EQ(OK, scan_refs(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats));

    std::vector<tree::FileNode*> act;
    collect(ra, act);
    const tree::FileNode* hello = nullptr;
    for (const auto* n : act)
        if (n->name_utf8() == "hello.txt")
            hello = n;
    ASSERT_NE(nullptr, hello);
    ASSERT_EQ(1u, hello->props()->extents.size());
    EXPECT_EQ(200 * 4096ull, hello->props()->extents[0].offset);
    /* and the container volume's thesis-grammar files are untouched too:
     * the container fixture only changes the og-grammar runs' targets */
    std::vector<u8> img2 = build_volume(true);
    RamDevice dev2(img2);
    tree::ResultTree tree2;
    auto* ra2 = tree2.add_root(u"Found");
    auto* rd2 = tree2.add_root(u"Deleted");
    auto* rl2 = tree2.add_root(u"Lost");
    RefsScanStats stats2;
    ASSERT_EQ(OK, scan_refs(&dev2, flags, nullptr, nullptr, nullptr, tree2, ra2, rd2, rl2, stats2));
    std::vector<tree::FileNode*> act2;
    collect(ra2, act2);
    const tree::FileNode* hello2 = nullptr;
    for (const auto* n : act2)
        if (n->name_utf8() == "hello.txt")
            hello2 = n;
    ASSERT_NE(nullptr, hello2);
    ASSERT_EQ(1u, hello2->props()->extents.size());
    EXPECT_EQ(200 * 4096ull, hello2->props()->extents[0].offset)
        << "thesis-grammar runs on the container volume: block 200 >= span 8 but "
           "flags 0x10 (not 0x40) never enter the container path";
}

} // namespace
} // namespace refs
} // namespace cr
