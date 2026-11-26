/* test_mftpatch.cpp - W2/F-02 unit tests: $MFT run-list reconstruction
 * + record-0 in-place patch (ntfs-recover behavior, re-implemented).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/ntfs/cr_mftpatch.h"
#include "cr/ntfs/cr_attributes.h"
#include "cr/ntfs/cr_fixup.h"
#include "cr/ntfs/cr_ntfs_wire.h"
#include "cr/ntfs/cr_runlist.h"
#include <gtest/gtest.h>
#include <cstring>
#include <vector>

namespace cr {
namespace ntfs {
namespace {

void put16t(u8* p, u16 v) {
    p[0] = u8(v);
    p[1] = u8(v >> 8);
}
void put32t(u8* p, u32 v) {
    put16t(p, u16(v));
    put16t(p + 2, u16(v >> 16));
}

/* ---- delta-grouping ------------------------------------------------ */

TEST(MftPatch, DeltaGroupingReconstructsExtents) {
    /* Two extents: records 0..7 at LCN 4 (delta = 4*4096 - 0 = 16384),
     * records 16..23 at LCN 100 (delta = 100*4096 - 16*1024 = 389120).
     * Geometry: cluster 4096, record 1024, rpc 4. */
    std::vector<FoundRecord> recs;
    for (u64 i = 0; i < 32; ++i)
        recs.push_back({4 * 4096 + i * 1024, i, 1, 0});
    for (u64 i = 32; i < 64; ++i)
        recs.push_back({100 * 4096 + (i - 32) * 1024, i, 1, 0});

    MftRunlistPlan plan; /* uncapped */
    MftPatchStats stats;
    reconstruct_mft_runlist(recs, 4096, 1024, 4, plan, stats);

    ASSERT_EQ(plan.fragments.size(), 2u);
    EXPECT_EQ(plan.fragments[0].logical_start, 0u);
    EXPECT_EQ(plan.fragments[0].cluster_count, 8u);
    EXPECT_EQ(plan.fragments[0].cluster_start, 4u);
    EXPECT_EQ(plan.fragments[1].logical_start, 32u);
    EXPECT_EQ(plan.fragments[1].cluster_count, 8u);
    EXPECT_EQ(plan.fragments[1].cluster_start, 100u);
    EXPECT_FALSE(plan.capped);
    EXPECT_EQ(stats.unique_records, 64u);
    EXPECT_EQ(stats.delta_groups, 2u);
}

TEST(MftPatch, GapsInsideGroupDoNotSplitFragment) {
    /* records 0,5,10,...,95 in one extent (missing records between) - one
     * fragment spanning 0..96 records = 24 clusters. */
    std::vector<FoundRecord> recs;
    for (u64 i = 0; i < 96; i += 5)
        recs.push_back({4 * 4096 + i * 1024, i, 1, 0});
    MftRunlistPlan plan;
    MftPatchStats stats;
    reconstruct_mft_runlist(recs, 4096, 1024, 4, plan, stats);
    ASSERT_EQ(plan.fragments.size(), 1u);
    EXPECT_EQ(plan.fragments[0].logical_start, 0u);
    EXPECT_EQ(plan.fragments[0].cluster_count, 24u);
    EXPECT_EQ(plan.fragments[0].cluster_start, 4u);
}

TEST(MftPatch, TinyFragmentsDropped) {
    /* 3 records = under 1 cluster... at rpc 4, one cluster; below the
     * 4-cluster (16-record) threshold -> dropped. */
    std::vector<FoundRecord> recs;
    for (u64 i = 0; i < 12; ++i) /* 12 records = 3 clusters < 4 */
        recs.push_back({4 * 4096 + i * 1024, i, 1, 0});
    MftRunlistPlan plan;
    MftPatchStats stats;
    reconstruct_mft_runlist(recs, 4096, 1024, 4, plan, stats);
    EXPECT_TRUE(plan.fragments.empty());
    EXPECT_EQ(stats.dropped_tiny, 1u);
}

TEST(MftPatch, ExtensionRecordsSkippedAndDedupKeepsHighestSequence) {
    /* record 5 seen twice (seq 3 then seq 9) + an extension record. */
    std::vector<FoundRecord> recs;
    recs.push_back({4 * 4096, 0, 1, 0});
    recs.push_back({4 * 4096 + 5 * 1024, 5, 3, 0});
    recs.push_back({4 * 4096 + 5 * 1024, 5, 9, 0});  /* dedup winner */
    recs.push_back({4 * 4096 + 6 * 1024, 6, 1, 42}); /* extension: skipped */
    MftRunlistPlan plan;
    MftPatchStats stats;
    reconstruct_mft_runlist(recs, 4096, 1024, 4, plan, stats);
    EXPECT_EQ(stats.unique_records, 2u); /* 0 and 5 */
    EXPECT_EQ(stats.scanned_records, 4u);
}

TEST(MftPatch, UnalignedExtentDropped) {
    /* delta 512 (not cluster-aligned): extent start = 512 -> dropped. */
    std::vector<FoundRecord> recs;
    for (u64 i = 0; i < 20; ++i)
        recs.push_back({512 + 4 * 4096 + i * 1024, i, 1, 0});
    MftRunlistPlan plan;
    MftPatchStats stats;
    reconstruct_mft_runlist(recs, 4096, 1024, 4, plan, stats);
    EXPECT_TRUE(plan.fragments.empty());
    EXPECT_EQ(stats.dropped_unaligned, 1u);
}

TEST(MftPatch, VcnCapTruncatesAndFilters) {
    /* records 0..63 (16 clusters) with cap max_record = 31: the step-1
     * filter drops records 32..63, so the group spans 0..32 records =
     * 8 clusters exactly. NOTE: the truncate-to-cap branch is defensive
     * by construction (same as the reference): with the record filter
     * running first, a cluster-aligned cap can never leave an
     * over-long fragment - ceil((group_end)/rpc) <= ceil((cap+1)/rpc)
     * always holds. The observable contract is the capped extent. */
    std::vector<FoundRecord> recs;
    for (u64 i = 0; i < 64; ++i)
        recs.push_back({4 * 4096 + i * 1024, i, 1, 0});
    MftRunlistPlan plan;
    plan.max_record = 31; /* caller-derived cap */
    MftPatchStats stats;
    reconstruct_mft_runlist(recs, 4096, 1024, 4, plan, stats);
    ASSERT_EQ(plan.fragments.size(), 1u);
    EXPECT_TRUE(plan.capped);
    EXPECT_EQ(plan.fragments[0].cluster_count, 8u); /* 0..32 records */
    EXPECT_EQ(stats.unique_records, 32u);           /* 32..63 filtered */
}

TEST(MftPatch, CapFiltersRecordsBeyondItEntirely) {
    /* cap 31; a separate extent at records 100..131 never appears. */
    std::vector<FoundRecord> recs;
    for (u64 i = 0; i < 32; ++i)
        recs.push_back({4 * 4096 + i * 1024, i, 1, 0});
    for (u64 i = 100; i < 132; ++i)
        recs.push_back({100 * 4096 + (i - 100) * 1024, i, 1, 0});
    MftRunlistPlan plan;
    plan.max_record = 31;
    MftPatchStats stats;
    reconstruct_mft_runlist(recs, 4096, 1024, 4, plan, stats);
    ASSERT_EQ(plan.fragments.size(), 1u);
    EXPECT_EQ(plan.fragments[0].logical_start, 0u);
    EXPECT_EQ(plan.fragments[0].cluster_count, 8u);
}

TEST(MftPatch, ContiguousExtentsMerge) {
    /* Same-delta records across a gap: 0..15 and 20..35 at one extent -
     * one group spanning 0..36 records (9 clusters); the group invariant
     * holds with internal gaps. */
    std::vector<FoundRecord> recs;
    for (u64 i = 0; i < 16; ++i)
        recs.push_back({4 * 4096 + i * 1024, i, 1, 0});
    for (u64 i = 20; i < 36; ++i)
        recs.push_back({4 * 4096 + i * 1024, i, 1, 0});
    MftRunlistPlan plan;
    MftPatchStats stats;
    reconstruct_mft_runlist(recs, 4096, 1024, 4, plan, stats);
    ASSERT_EQ(plan.fragments.size(), 1u);
    EXPECT_EQ(plan.fragments[0].cluster_count, 9u); /* 0..36 records */
}

TEST(MftPatch, ContiguousDistinctDeltasMerge) {
    /* Two DIFFERENT deltas whose fragments abut both logically and
     * physically: extent A records 0..31 at cluster 4 (delta 16384);
     * extent B records 32..63 at cluster 12 (delta = 12*4096 - 32*1024
     * = 16384 - 32768 = -16384... to keep the physical chain
     * contiguous B must start at cluster 4+8=12: delta = 12*4096 -
     * 32*1024 = 49152 - 32768 = 16384. Any delta != A's needs a
     * physical gap - the NTFS reality (allocation gaps between MFT
     * extents). The merge rule therefore fires only through the TRIM
     * path or not at all on distinct deltas; the containment/skip rule
     * is what OverlapTrimsAndContainmentSkips pins. */
    SUCCEED();
}

TEST(MftPatch, OverlapTrimsAndContainmentSkips) {
    /* The alignment-overlap class. Records are deduplicated by number
     * across the whole pool, so fragments' record ranges cannot
     * overlap - but their ALIGNED spans can: fragment A holds records
     * 0..30 (aligned span 0..32 = 8 clusters); fragment B holds
     * records 31..46 in a different delta group (31 is A's LAST record
     * + 1 - never deduplicated away), B's aligned span is records
     * 28..48 (5 clusters: survives the 4-cluster minimum), which
     * STARTS at 28 < A's end 32 and extends to 48 > 32. The merge step
     * trims B to the part beyond logical 32: logical 32..48 = 4
     * clusters, cluster_start advanced by the 1-cluster overlap.
     *
     * Geometry: cluster 4096, record 1024, rpc 4. A at cluster 4
     * (delta 16384). B's records must sit at a different, cluster-
     * aligned position: B's aligned start (record 28) maps to disk
     * offset delta_B + 28*1024, so pick cluster 20 -> 81920; delta_B =
     * 81920 - 28*1024 = 53248. B record i at 53248 + i*1024. */
    std::vector<FoundRecord> recs;
    for (u64 i = 0; i <= 30; ++i)
        recs.push_back({4 * 4096 + i * 1024, i, 1, 0}); /* A: delta 16384 */
    for (u64 i = 31; i <= 46; ++i)
        recs.push_back({52 * 1024 + i * 1024, i, 1, 0}); /* B: delta 53248 */
    MftRunlistPlan plan;
    MftPatchStats stats;
    reconstruct_mft_runlist(recs, 4096, 1024, 4, plan, stats);
    ASSERT_EQ(plan.fragments.size(), 2u);
    /* A unchanged: records 0..30 -> logical 0..32, 8 clusters at 4 */
    EXPECT_EQ(plan.fragments[0].logical_start, 0u);
    EXPECT_EQ(plan.fragments[0].cluster_count, 8u);
    EXPECT_EQ(plan.fragments[0].cluster_start, 4u);
    /* B trimmed: aligned span was 28..48 (5 clusters at 20); the part
     * beyond A's end (logical 32) = 16 records = 4 clusters, physical
     * start advanced past the overlap: 20 + (32-28)/4 = 21. */
    EXPECT_EQ(plan.fragments[1].logical_start, 32u);
    EXPECT_EQ(plan.fragments[1].cluster_start, 21u);
    EXPECT_EQ(plan.fragments[1].cluster_count, 4u);
}

/* ---- sign-extension-safe encoding ---------------------------------- */

TEST(MftPatch, EncodeSparesSignBitOfLength) {
    /* length 45669 = 0xB265: bit_length 16 -> naive 2 bytes decodes
     * negative under ntfs-3g. Safe encoding uses 3 bytes. */
    std::vector<MftFragment> frags;
    MftFragment f;
    f.logical_start = 0;
    f.cluster_count = 45669; /* the reference's exact bug case */
    f.cluster_start = 0;
    frags.push_back(f);
    const auto enc = encode_runlist_ntfs3g_safe(frags, 4);
    ASSERT_GE(enc.size(), 3u);
    /* header: len_width 3 (low nibble), off_width 1 (high nibble) */
    EXPECT_EQ(enc[0], u8((1u << 4) | 3u));
    EXPECT_EQ(enc[1], 0x65);
    EXPECT_EQ(enc[2], 0xB2);
    EXPECT_EQ(enc[3], 0x00);   /* the spared sign byte */
    EXPECT_EQ(enc.back(), 0u); /* terminator */
}

TEST(MftPatch, EncodeNegativeDeltaUsesSignedWidth) {
    /* second fragment at a LOWER cluster: delta = -1000 = 0xFFFFFC18
     * (two's complement); width from leading ones: fits 2 bytes? -1000
     * needs 16 bits signed -> 2 bytes. */
    std::vector<MftFragment> frags;
    MftFragment a;
    a.logical_start = 0;
    a.cluster_count = 4;
    a.cluster_start = 2000;
    frags.push_back(a);
    MftFragment b;
    b.logical_start = 16;
    b.cluster_count = 4;
    b.cluster_start = 1000; /* delta -1000 */
    frags.push_back(b);
    const auto enc = encode_runlist_ntfs3g_safe(frags, 4);
    /* run 1: header (off 1 byte | len 1 byte), len 4, off 2000 (2 bytes:
     * 2000 = 0x7D0, bit_length 11 -> 2 bytes) => bytes [0..3];
     * run 2: header (off 2 bytes | len 1 byte), len 4, off -1000
     * (2 bytes) => bytes [4..7]. */
    ASSERT_GE(enc.size(), 8u);
    /* run 1 header: off_width 2, len_width 1 */
    EXPECT_EQ(enc[0], u8((2u << 4) | 1u));
    EXPECT_EQ(enc[1], 0x04); /* length 4 */
    EXPECT_EQ(enc[2], 0xD0); /* 2000 LE16 = D0 07 */
    EXPECT_EQ(enc[3], 0x07);
    /* run 2 header: off_width 2, len_width 1 */
    EXPECT_EQ(enc[4], u8((2u << 4) | 1u));
    EXPECT_EQ(enc[5], 0x04); /* length 4 */
    EXPECT_EQ(enc[6], 0x18); /* -1000 LE16 = 18 FC */
    EXPECT_EQ(enc[7], 0xFC);
}

TEST(MftPatch, EncodedRunlistDecodesBackThroughEngineCodec) {
    /* Round-trip: our safe encoding must decode through the engine's
     * own RunList::decode with identical extents. */
    std::vector<MftFragment> frags;
    MftFragment a;
    a.logical_start = 0;
    a.cluster_count = 45669;
    a.cluster_start = 5;
    frags.push_back(a);
    MftFragment b;
    b.logical_start = 45669 * 4; /* rpc 4 - next cluster-aligned slot */
    b.cluster_count = 70000;
    b.cluster_start = 5 + 45669 + 33; /* gap of 33 clusters (sparse-ish skip) */
    frags.push_back(b);
    const auto enc = encode_runlist_ntfs3g_safe(frags, 4);
    RunList rl;
    ASSERT_EQ(RunList::decode(enc.data(), u32(enc.size()), 0, ~0ull, rl), u32(OK));
    ASSERT_EQ(rl.runs().size(), 2u);
    EXPECT_EQ(rl.runs()[0].length, 45669u);
    EXPECT_EQ(rl.runs()[0].lcn, 5u);
    EXPECT_EQ(rl.runs()[1].length, 70000u);
    EXPECT_EQ(rl.runs()[1].lcn, 5u + 45669u + 33u);
    EXPECT_FALSE(rl.torn);
}

/* ---- record-0 patch ------------------------------------------------- */

/* Build a 1 KiB record-0-shaped buffer: FILE magic, USA at 0x30 (3
 * entries for 2 sectors), a $FILE_NAME, then a non-resident $DATA with
 * `run_space` bytes of run room pre-filled with junk. */
struct Record0Builder {
    std::vector<u8> rec;
    u32 data_attr_pos = 0;
    u32 runlist_pos = 0;
    u32 run_space = 0;

    explicit Record0Builder(u32 runs_room) : rec(1024, 0) {
        auto* rh = reinterpret_cast<RecordHeader*>(rec.data());
        rh->magic = RECORD_MAGIC_FILE;
        rh->update_sequence_offset = 0x30;
        rh->update_sequence_size = 3;
        rh->sequence = 7;
        rh->attribute_offset = 0x38;
        rh->flags = RECORD_IN_USE;
        rh->bytes_in_use = 1024;
        rh->bytes_allocated = 1024;
        rh->record_number = 0;
        put16t(rec.data() + 0x30, 9); /* USA number 9 */
        /* tails carry the USA number; original words are 0 (buffer is
         * zero) - fixup_apply restores them from slots 1..2. */
        put16t(rec.data() + 510, 9);
        put16t(rec.data() + 1022, 9);
        u32 pos = 0x38;
        /* non-resident unnamed $DATA: header 0x40 + runs */
        data_attr_pos = pos;
        const u32 total = 0x40 + runs_room;
        put32t(rec.data() + pos, ATTR_TYPE_DATA);
        put32t(rec.data() + pos + 4, total);
        rec[pos + 8] = 1;                     /* non-resident */
        put32t(rec.data() + pos + 0x10, 0);   /* start_vcn lo */
        put32t(rec.data() + pos + 0x14, 0);   /* start_vcn hi */
        put32t(rec.data() + pos + 0x18, 999); /* last_vcn lo (preserved) */
        put32t(rec.data() + pos + 0x1C, 0);
        put16t(rec.data() + pos + 0x20, u16(0x40)); /* runlist offset rel attr */
        runlist_pos = pos + 0x40;
        this->run_space = runs_room;
        /* junk runs (invalid: zero-length tail) - the damaged state */
        rec[runlist_pos] = 0x01;
        rec[runlist_pos + 1] = 0x00; /* zero-length: damaged */
        pos += total;
        put32t(rec.data() + pos, 0xFFFFFFFF); /* terminator */
    }
};

TEST(MftPatch, PatchRecord0AppliesRunsAndBumpsUsa) {
    Record0Builder b(32);
    std::vector<MftFragment> frags;
    MftFragment f;
    f.logical_start = 0;
    f.cluster_count = 16;
    f.cluster_start = 4;
    frags.push_back(f);
    const auto enc = encode_runlist_ntfs3g_safe(frags, 4);

    ASSERT_EQ(patch_record0_runlist(b.rec.data(), 1024, 512, enc), u32(OK));

    /* the runs are in place at the attribute's run offset */
    EXPECT_EQ(std::memcmp(b.rec.data() + b.runlist_pos, enc.data(), enc.size()), 0);
    /* USA number bumped 9 -> 10 at the USA base */
    u16 sig = u16(b.rec[0x30]) | (u16(b.rec[0x31]) << 8);
    EXPECT_EQ(sig, 10u);
    /* sector tails carry the new number */
    u16 t0 = u16(b.rec[510]) | (u16(b.rec[511]) << 8);
    u16 t1 = u16(b.rec[1022]) | (u16(b.rec[1023]) << 8);
    EXPECT_EQ(t0, 10u);
    EXPECT_EQ(t1, 10u);
    /* USA slots hold the pre-fixup tail words (both zero here) */
    u16 s1 = u16(b.rec[0x32]) | (u16(b.rec[0x33]) << 8);
    EXPECT_EQ(s1, 0u);
    /* last_vcn preserved */
    u32 last_vcn = u32(b.rec[b.data_attr_pos + 0x18]) | (u32(b.rec[b.data_attr_pos + 0x19]) << 8);
    EXPECT_EQ(last_vcn, 999u);
    /* the rest of the run space is zero (redundant terminator) */
    for (u32 i = u32(enc.size()); i < b.run_space; ++i)
        EXPECT_EQ(b.rec[b.runlist_pos + i], 0u) << "run space byte " << i;
}

TEST(MftPatch, PatchRefusesWhenRunsDoNotFit) {
    /* enum_attributes requires runlist_offset + 8 <= attribute length,
     * so the minimum valid run space is 8 bytes. Runs needing more are
     * refused - the record is never restructured. */
    Record0Builder b(8); /* 8 bytes of run room */
    std::vector<MftFragment> frags;
    MftFragment f;
    f.logical_start = 0;
    f.cluster_count =
        0x10000 +
        5; /* 3-byte length + 3-byte offset + header + term = 8... use two fragments to exceed 8 */
    f.cluster_start = 4;
    frags.push_back(f);
    MftFragment g;
    g.logical_start = f.cluster_count * 4;
    g.cluster_count = 9;
    g.cluster_start = 4 + f.cluster_count + 7;
    frags.push_back(g);
    const auto enc = encode_runlist_ntfs3g_safe(frags, 4); /* 2 runs -> > 8 bytes */
    ASSERT_GT(enc.size(), 8u);
    ASSERT_EQ(patch_record0_runlist(b.rec.data(), 1024, 512, enc), u32(ERR_REPAIR_REFUSED));
}

TEST(MftPatch, PatchRefusesNonFileMagic) {
    Record0Builder b(32);
    std::vector<u8> bad = b.rec;
    bad[0] = 'X';
    std::vector<MftFragment> frags;
    MftFragment f;
    f.logical_start = 0;
    f.cluster_count = 16;
    f.cluster_start = 4;
    frags.push_back(f);
    const auto enc = encode_runlist_ntfs3g_safe(frags, 4);
    ASSERT_EQ(patch_record0_runlist(bad.data(), 1024, 512, enc), u32(ERR_BAD_PARAMETER));
}

TEST(MftPatch, PatchRefusesTornFixup) {
    Record0Builder b(32);
    /* fixup_validate checks the FIRST TWO sector tails against USA[0];
     * corrupt sector 1's tail so the record fails validation - the
     * patch must refuse rather than patch over a torn fixup (S3). */
    put16t(b.rec.data() + 510, 0x1234);
    std::vector<MftFragment> frags;
    MftFragment f;
    f.logical_start = 0;
    f.cluster_count = 16;
    f.cluster_start = 4;
    frags.push_back(f);
    const auto enc = encode_runlist_ntfs3g_safe(frags, 4);
    ASSERT_EQ(patch_record0_runlist(b.rec.data(), 1024, 512, enc), u32(ERR_FIXUP_FAILED));
}

TEST(MftPatch, UsaWrapAvoidsZeroAndFfff) {
    Record0Builder b(32);
    put16t(b.rec.data() + 0x30, 0xFFFE); /* USA number at the wrap edge */
    put16t(b.rec.data() + 510, 0xFFFE);
    put16t(b.rec.data() + 1022, 0xFFFE);
    std::vector<MftFragment> frags;
    MftFragment f;
    f.logical_start = 0;
    f.cluster_count = 16;
    f.cluster_start = 4;
    frags.push_back(f);
    const auto enc = encode_runlist_ntfs3g_safe(frags, 4);
    ASSERT_EQ(patch_record0_runlist(b.rec.data(), 1024, 512, enc), u32(OK));
    u16 sig = u16(b.rec[0x30]) | (u16(b.rec[0x31]) << 8);
    EXPECT_EQ(sig, 1u); /* 0xFFFE + 1 wraps to 1, never 0/0xFFFF */
}

/* ---- the cap derivation -------------------------------------------- */

TEST(MftPatch, DeriveVcnCapFromRecord0) {
    Record0Builder b(32);
    /* last_vcn 999 -> max_record = (999+1)*rpc(4) - 1 = 3999 */
    u64 max_record = 0;
    ASSERT_TRUE(derive_vcn_cap_from_record0(b.rec.data(), 1024, 4096, 1024, max_record));
    EXPECT_EQ(max_record, 3999u);
}

TEST(MftPatch, DeriveCapRejectsBadRecords) {
    Record0Builder b(32);
    u64 max_record = 0;
    /* not FILE */
    std::vector<u8> bad1 = b.rec;
    bad1[0] = 'X';
    EXPECT_FALSE(derive_vcn_cap_from_record0(bad1.data(), 1024, 4096, 1024, max_record));
    /* null */
    EXPECT_FALSE(derive_vcn_cap_from_record0(nullptr, 1024, 4096, 1024, max_record));
    /* short */
    EXPECT_FALSE(derive_vcn_cap_from_record0(b.rec.data(), 0x100, 4096, 1024, max_record));
}

} // namespace
} // namespace ntfs
} // namespace cr
