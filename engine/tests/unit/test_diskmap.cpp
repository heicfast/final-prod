/*
 * test_diskmap.cpp - the S-009 free-overlay hardening suite (session 9's
 * port onto the landed storage::FreeMap implementation).
 *
 * Complements the landed tests: the range_free edge matrix, the
 * REC_ALLOC_MAP record round-trip + hostile-input rejection, the
 * zoom-window truth (before/inside/past the map's coverage), and the
 * real-$Bitmap NTFS seam (test_ntfs_volume.cpp).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/tree/cr_diskmap.h"
#include "cr/session/cr_records.h"
#include "cr/tree/cr_node.h"
#include "cr/tree/cr_snapshot.h"
#include "cr/storage/cr_device.h"

#include <gtest/gtest.h>

using namespace cr;
using cr::storage::FreeMap;

namespace {

FreeMap all_free_map(u64 base, u32 cluster_bytes, u64 clusters) {
    FreeMap fm;
    fm.base_byte = base;
    fm.cluster_bytes = cluster_bytes;
    fm.cluster_count = clusters;
    fm.bits.assign(size_t((clusters + 7) / 8), 0xFF);
    return fm;
}

} // namespace

/* The truth gate: range_free answers free ONLY for spans fully inside the
 * map's coverage with every covering cluster's bit set. Before-base,
 * past-end, zero-length, and straddling spans all read NOT-free (unknown
 * never becomes free - the cr_freemap.h contract). */
TEST(DiskMap, RangeFreeEdgeMatrix) {
    FreeMap fm = all_free_map(4096, 4096, 4); /* [4096, 20480) free */

    EXPECT_TRUE(fm.range_free(4096, 4096));   /* cluster 0 */
    EXPECT_TRUE(fm.range_free(8192, 8192));   /* clusters 1..2 */
    EXPECT_TRUE(fm.range_free(4096, 16384));  /* the whole map */
    EXPECT_FALSE(fm.range_free(0, 4096));     /* before base */
    EXPECT_FALSE(fm.range_free(0, 8192));     /* straddles base */
    EXPECT_FALSE(fm.range_free(4096, 20480)); /* past the end */
    EXPECT_FALSE(fm.range_free(20480, 4096)); /* entirely past */
    EXPECT_FALSE(fm.range_free(4096, 0));     /* empty span: no claim */
    /* sub-cluster span: the containing cluster's truth governs (the map
     * is cluster-granular; the disk map's cells are never finer) */
    EXPECT_TRUE(fm.range_free(4096, 1));
    EXPECT_TRUE(fm.range_free(4097, 2048)); /* still inside cluster 0 */

    /* clear a middle bit: spans crossing it stop being free */
    fm.bits[0] = 0b00001101;                 /* cluster 1's bit cleared */
    EXPECT_TRUE(fm.range_free(4096, 4096));  /* cluster 0 alone */
    EXPECT_FALSE(fm.range_free(4096, 8192)); /* crosses cluster 1 */
    EXPECT_TRUE(fm.range_free(12288, 8192)); /* clusters 2..3 */

    /* an invalid map answers nothing */
    FreeMap bad;
    EXPECT_FALSE(bad.range_free(0, 512));
    bad.cluster_bytes = 512;
    EXPECT_FALSE(bad.range_free(0, 512)); /* zero clusters: still invalid */
}

/* The REC_ALLOC_MAP payload round-trip: the downsampled map serializes
 * and reloads bit-exact; hostile inputs (truncation, size lies, an
 * absent map) are format errors, never half-maps. */
TEST(DiskMap, FreeMapRecordRoundTripAndRejection) {
    FreeMap fm = all_free_map(1 << 20, 4096, 300);
    /* punch holes: clusters 5, 100..103 not free */
    for (u64 c : {u64(5), u64(100), u64(101), u64(102), u64(103)})
        fm.bits[size_t(c >> 3)] &= u8(~(1u << (c & 7)));

    std::vector<u8> blob;
    ASSERT_EQ(session::serialize_free_map(fm, blob), OK);
    FreeMap back;
    ASSERT_EQ(session::deserialize_free_map(blob.data(), blob.size(), back), OK);
    EXPECT_EQ(back.base_byte, fm.base_byte);
    EXPECT_EQ(back.cluster_bytes, fm.cluster_bytes);
    EXPECT_EQ(back.cluster_count, fm.cluster_count);
    EXPECT_EQ(back.bits, fm.bits);
    for (u64 c = 0; c < 300; ++c) {
        const bool want = c != 5 && !(c >= 100 && c <= 103);
        EXPECT_EQ(back.is_free(c), want) << "cluster " << c;
    }

    /* truncation at every prefix boundary is a clean rejection */
    for (size_t cut = 0; cut < blob.size(); ++cut)
        EXPECT_NE(session::deserialize_free_map(blob.data(), cut, back), u32(OK))
            << "cut at " << cut;

    /* a bits_len lie (cluster_count disagrees with the packed size) */
    std::vector<u8> lie = blob;
    lie[24] = 0xEE; /* corrupt cluster_count low byte */
    EXPECT_NE(session::deserialize_free_map(lie.data(), lie.size(), back), u32(OK));

    /* an absent map never serializes (the record is optional-by-presence) */
    FreeMap empty;
    std::vector<u8> none;
    EXPECT_NE(session::serialize_free_map(empty, none), u32(OK));
}

/* The zoom-window truth: cells before, inside, and past the map's
 * coverage render Unknown/Free/Unknown at the VIEW's own granularity -
 * the map proves nothing outside its span. */
TEST(DiskMap, FreeOverlayHonorsZoomWindow) {
    FreeMap fm = all_free_map(1 << 20, 4096, 256); /* [1 MiB, 2 MiB) */

    tree::ResultTree tree; /* empty tree: no competing claims */
    viz::DiskMapInputs in;
    in.slice_start = 0;
    in.slice_bytes = 1024 * 4096; /* [0, 4 MiB) at 4096-byte cells */
    in.cluster_bytes = 4096;
    in.free_map = &fm;
    viz::DiskMapGeometry g;
    std::vector<u8> alloc;
    std::vector<std::pair<u64, u64>> bad;
    ASSERT_EQ(viz::build_disk_map(in, tree, g, &alloc, nullptr, nullptr, nullptr, bad, 0, nullptr),
              OK);
    ASSERT_EQ(alloc.size(), 1024u);
    EXPECT_EQ(alloc[0], viz::kCellAllocUnknown);   /* before the map */
    EXPECT_EQ(alloc[255], viz::kCellAllocUnknown); /* still before it */
    EXPECT_EQ(alloc[256], viz::kCellAllocFree);    /* the map's first span */
    EXPECT_EQ(alloc[511], viz::kCellAllocFree);    /* the map's last span */
    EXPECT_EQ(alloc[512], viz::kCellAllocUnknown); /* past the map */
}

/* The downsample preserves the raw truth at the view's granularity: a
 * not-free run in the raw clusters stays not-free in the cells that
 * cover it, and free runs stay free (the AND/coverage rules are pinned
 * by RangeFreeEdgeMatrix at the cluster scale and by the landed
 * uncovered-tail test at the view scale). */
TEST(DiskMap, DownsamplePreservesRawTruth) {
    /* raw map: 512-byte clusters over [0, 64 KiB); clusters 4..7 NOT free */
    FreeMap raw = all_free_map(0, 512, 128);
    for (u64 c = 4; c < 8; ++c)
        raw.bits[size_t(c >> 3)] &= u8(~(1u << (c & 7)));

    FreeMap down;
    ASSERT_EQ(viz::downsample_free_map(raw, 0, 64 * 1024, down), OK);
    ASSERT_EQ(down.cluster_bytes, 512u); /* small view: no growth */
    ASSERT_EQ(down.cluster_count, 128u);
    for (u64 c = 0; c < 128; ++c) {
        const bool want = !(c >= 4 && c <= 7);
        EXPECT_EQ(down.is_free(c), want) << "cluster " << c;
    }
}

/* S-009 hardening (the finding's rule, verbatim): "cells the bitmap
 * marks free and NO FOUND-CONTENT covers -> kCellAllocFree". The first
 * landing painted every still-unknown bitmap-free cell Free, including
 * cells where DELETED or CARVED content was found - deleted files live
 * in free space, and painting their cells plain-free hides the one
 * thing a recovery UI must foreground. This test pins all three cell
 * fates under a hostile all-free map:
 *   - ACTIVE structured file cell -> USED (never downgraded)
 *   - DELETED file's cell         -> UNKNOWN (content, not plain-free)
 *   - empty free cell             -> FREE
 */
TEST(DiskMap, FreeNeverMarksFoundContentCells) {
    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    auto* deleted = tree.add_root(u"Deleted");
    auto add = [&](tree::FullNode* parent, const char16_t* name, u64 off, u64 len,
                   tree::DeletedState st) {
        auto* p = new tree::FileProperties();
        p->name = name;
        p->logical_size = len;
        p->extents.push_back({off, len});
        tree::FileNode* n = tree.add_file(parent, p);
        p->release();
        n->set_origin(tree::Origin::Structured);
        n->set_deleted_state(st);
        return n;
    };
    add(found, u"live.txt", 0, 4096, tree::DeletedState::Active);
    add(deleted, u"gone.txt", 100 * 4096, 4096, tree::DeletedState::Deleted);

    /* a hostile map: EVERYTHING claims free (the stale-bitmap case) */
    FreeMap fm = all_free_map(0, 4096, 256); /* [0, 1 MiB) all-free */

    viz::DiskMapInputs in;
    in.slice_start = 0;
    in.slice_bytes = 256 * 4096;
    in.cluster_bytes = 4096;
    in.free_map = &fm;
    viz::DiskMapGeometry g;
    std::vector<u8> alloc;
    std::vector<std::pair<u64, u64>> bad;
    ASSERT_EQ(viz::build_disk_map(in, tree, g, &alloc, nullptr, nullptr, nullptr, bad, 0, nullptr),
              OK);
    ASSERT_EQ(alloc.size(), 256u);
    EXPECT_EQ(alloc[0], viz::kCellAllocUsed)
        << "an active file's cell is USED even under a hostile all-free map";
    EXPECT_EQ(alloc[100], viz::kCellAllocUnknown)
        << "a deleted file's cell stays Unknown (found content), never plain-free";
    EXPECT_EQ(alloc[50], viz::kCellAllocFree) << "an empty cell inside the map is FREE";
}

TEST(DiskMap, OverlayBudgetGrowsCellSize) {
    /* N-057: the 64 MiB overlay budget's pow-2 cell growth was untested.
     * Exactly at budget: cell stays the cluster size; one cell past it:
     * the cell doubles (memory budget honored - the alloc/cov vectors
     * cap at the budget regardless of slice size). */
    tree::ResultTree tree; /* empty tree: no extent claims */
    viz::DiskMapInputs in;
    in.slice_start = 0;
    in.cluster_bytes = 512;
    viz::DiskMapGeometry g;
    std::vector<u8> cov;

    /* exactly 64 MiB cells at 512 B: total/cell == kOverlayBudgetBytes ->
     * NOT over budget -> cell stays 512 (choose_cell_bytes' loop is >) */
    std::vector<std::pair<u64, u64>> bad;
    in.slice_bytes = (64ull << 20) * 512;
    ASSERT_EQ(viz::build_disk_map(in, tree, g, nullptr, &cov, nullptr, nullptr, bad, 0, nullptr),
              OK);
    EXPECT_EQ(g.cell_bytes, 512u) << "at-budget: no growth";
    EXPECT_EQ(g.cell_count, (64ull << 20));
    cov.shrink_to_fit();

    /* one 512-byte sector MORE: total/cell == budget+1 -> cell doubles */
    in.slice_bytes = (64ull << 20) * 512 + 512;
    ASSERT_EQ(viz::build_disk_map(in, tree, g, nullptr, &cov, nullptr, nullptr, bad, 0, nullptr),
              OK);
    EXPECT_EQ(g.cell_bytes, 1024u) << "one cell past budget: the cell doubles";
    EXPECT_EQ(g.cell_count, (64ull << 20) / 2 + 1);
    /* the overlay vectors stay within the budget */
    EXPECT_LE(cov.size(), size_t(64ull << 20));
}

/* The hostile-map precedence + downsample-conservatism proof (moved
 * from test_cr_ui.cpp: a pure core-builder test - the cr_ui suite
 * links ONLY the shared C-ABI surface on MSVC, which does not export
 * the C++ core; this test never touched the ABI). */

TEST(DiskMap, FreeNeverOverridesUsedOrBadAndDownsampleIsConservative) {
    /* Pure-builder precedence proof with a HOSTILE map: the bitmap
     * claims every cluster free while a structured-active extent covers
     * one cell and a read-error span covers another. USED and FsBad are
     * the stricter claims - free must never override either. Plus the
     * persistence downsample: cells only partially covered by the raw
     * map stay NOT free (unknown is not free). */
    using namespace cr;
    tree::ResultTree t;
    tree::FullNode* root = t.add_root(u"V");
    auto* p = new tree::FileProperties();
    p->record_ref = 1;
    p->name = u"used.bin";
    p->logical_size = 512;
    p->extents.push_back({2 * 512, 512}); /* cell 2 */
    tree::FileNode* f = t.add_file(root, p);
    ASSERT_NE(f, nullptr);
    p->release();

    storage::FreeMap fm; /* claims ALL of [0, 8 KiB) free */
    fm.base_byte = 0;
    fm.cluster_bytes = 512;
    fm.cluster_count = 16;
    fm.bits.assign(2, 0xFF);

    ReadErrorMap errors; /* a bad span over cell 5 */
    errors.add(5 * 512, 512, 3);

    viz::DiskMapInputs in;
    in.slice_start = 0;
    in.slice_bytes = 16 * 512;
    in.cluster_bytes = 512;
    in.free_map = &fm;
    in.errors = &errors;
    viz::DiskMapGeometry geom;
    std::vector<u8> alloc;
    std::vector<std::pair<u64, u64>> bad;
    u32 bad_needed = 0;
    ASSERT_EQ(
        viz::build_disk_map(in, t, geom, &alloc, nullptr, nullptr, nullptr, bad, 8, &bad_needed),
        u32(OK));
    ASSERT_EQ(geom.cell_count, 16u);
    EXPECT_EQ(alloc[2], 1u) << "USED is the stricter claim - hostile-free must not win";
    EXPECT_EQ(alloc[5], 3u) << "FsBad (error span) is the stricter claim";
    EXPECT_EQ(alloc[6], 2u) << "plain free cell paints FREE";
    EXPECT_EQ(alloc[15], 2u);
    EXPECT_EQ(bad_needed, 1u);

    /* downsample: the raw map covers [0, 8 KiB) but the view is 12 KiB
     * -> cells 16..23 (of the 24 half-KiB cells... at 512-byte cells)
     * are NOT covered and must stay not-free even in the raw map's own
     * tail; the covered cells stay free. */
    storage::FreeMap down;
    ASSERT_EQ(viz::downsample_free_map(fm, 0, 12 * 1024, down), u32(OK));
    ASSERT_EQ(down.cluster_count, 24u); /* 12 KiB at 512-byte cells */
    for (u64 k = 0; k < 16; ++k)
        EXPECT_TRUE(down.is_free(k)) << "covered free clusters stay free after downsample";
    for (u64 k = 16; k < 24; ++k)
        EXPECT_FALSE(down.is_free(k)) << "uncovered tail is unknown, never free";
}
