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
    EXPECT_TRUE(fm.range_free(4097, 2048));   /* still inside cluster 0 */

    /* clear a middle bit: spans crossing it stop being free */
    fm.bits[0] = 0b00001101; /* cluster 1's bit cleared */
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
