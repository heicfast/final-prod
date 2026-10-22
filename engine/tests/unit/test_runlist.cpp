/* RunList codec tests - nibble math, sparse runs, signed deltas (F6 family). */
#include <gtest/gtest.h>
#include "cr/ntfs/cr_runlist.h"
#include <cstring>

using namespace cr;
using namespace cr::ntfs;

TEST(RunList, SingleRunDecode) {
    /* len 0x1000 at LCN 0x1000: header 0x22 (len_w 2, off_w 2) */
    u8 raw[] = {0x22, 0x00, 0x10, 0x00, 0x10, 0x00};
    RunList rl;
    ASSERT_EQ(RunList::decode(raw, sizeof(raw), 0, 1 << 20, rl), OK);
    ASSERT_EQ(rl.runs().size(), 1u);
    const DataRun& r = rl.runs()[0];
    EXPECT_EQ(r.vcn, 0u);
    EXPECT_EQ(r.length, 0x1000u);
    EXPECT_EQ(r.lcn, 0x1000u);
    EXPECT_FALSE(r.sparse);
}

TEST(RunList, NegativeDelta) {
    /* run1: LCN 0x5000 len 0x10 ; run2: LCN 0x3000 len 0x10 (delta -0x2000) */
    /* spec order: header low nibble = len width, high = off width */
    u8 fixed[] = {0x21, 0x10, 0x00, 0x50, /* len 0x10, off +0x5000 (LE 00 50) */
                  0x21, 0x10, 0x00, 0xE0, /* len 0x10, off -0x2000 (0xE000 2s-complement) */
                  0x00};
    RunList rl;
    ASSERT_EQ(RunList::decode(fixed, sizeof(fixed), 0, 1 << 20, rl), OK);
    ASSERT_EQ(rl.runs().size(), 2u);
    EXPECT_EQ(rl.runs()[1].lcn, 0x3000u);
    EXPECT_EQ(rl.total_clusters(), 0x20u);
}

TEST(RunList, SparseRun) {
    /* run: len 0x10 at LCN 0x100 (off_w 2); sparse hole: len 0x20 (off_w 0) */
    u8 raw[] = {0x21, 0x10, 0x00, 0x01, /* header 0x21: len 0x10, off 0x0100 */
                0x01, 0x20,             /* sparse: header (0<<4)|1, len 0x20 */
                0x00};
    RunList rl;
    ASSERT_EQ(RunList::decode(raw, sizeof(raw), 0, 1 << 20, rl), OK);
    ASSERT_EQ(rl.runs().size(), 2u);
    EXPECT_TRUE(rl.runs()[1].sparse);
    EXPECT_EQ(rl.runs()[1].lcn, 0u);
    EXPECT_EQ(rl.runs()[1].vcn, 0x10u);
    /* vcn lookup inside the hole reports sparse */
    bool sparse = false;
    u64 lcn = 99;
    EXPECT_TRUE(rl.vcn_to_lcn(0x15, &lcn, &sparse));
    EXPECT_TRUE(sparse);
    /* lcn math on real run */
    EXPECT_TRUE(rl.vcn_to_lcn(0x5, &lcn, &sparse));
    EXPECT_FALSE(sparse);
    EXPECT_EQ(lcn, 0x105u);
}

TEST(RunList, TerminatesAndRejectsTruncated) {
    u8 raw[] = {0x11, 0x10, 0x00}; /* header promises 1+1 bytes but cut */
    RunList rl;
    EXPECT_EQ(RunList::decode(raw, sizeof(raw), 0, 1 << 20, rl), ERR_TRUNCATED);
}

TEST(RunList, ZeroLengthRunRejected) {
    u8 raw[] = {0x11, 0x00, 0x00, 0x00}; /* len 0 invalid */
    RunList rl;
    EXPECT_EQ(RunList::decode(raw, sizeof(raw), 0, 1 << 20, rl), ERR_BAD_PARAMETER);
}

TEST(RunList, MaxClusterGuard) {
    u8 raw[] = {0x11, 0x10, 0x00, 0x01};
    RunList rl;
    /* accumulated 0x10 > max 0xF */
    EXPECT_EQ(RunList::decode(raw, sizeof(raw), 0, 0xF, rl), ERR_BAD_PARAMETER);
}

TEST(RunList, EncodeDecodeRoundTrip) {
    RunList a;
    DataRun r1{0, 0x21, 0, 0x400, false};
    DataRun r2{0x21, 0x11, 0, 0x5000, false};
    DataRun r3{0x32, 0x40, 0, 0, true}; /* sparse hole */
    DataRun r4{0x72, 0x3, 0, 0x6000, false};
    a.runs() = {r1, r2, r3, r4};
    std::vector<u8> enc = a.encode();
    RunList b;
    ASSERT_EQ(RunList::decode(enc.data(), u32(enc.size()), 0, 1 << 30, b), OK);
    ASSERT_EQ(b.runs().size(), 4u);
    EXPECT_EQ(b.runs()[0].lcn, 0x400u);
    EXPECT_EQ(b.runs()[1].lcn, 0x5000u);
    EXPECT_EQ(b.runs()[1].vcn, 0x21u);
    EXPECT_TRUE(b.runs()[2].sparse);
    EXPECT_EQ(b.runs()[3].lcn, 0x6000u);
    EXPECT_EQ(b.runs()[3].vcn, 0x72u);
}

TEST(RunList, SignedWidthEdgeCases) {
    /* deltas crossing the 127/-128 and 32767/-32768 boundaries must not
     * truncate silently (the encode bug fixed this session) */
    for (i64 delta : {i64(127), i64(128), i64(-128), i64(-129), i64(255), i64(-256), i64(32767),
                      i64(32768), i64(-32768), i64(-32769)}) {
        RunList a;
        DataRun r1{0, 5, 0, 100, false};
        DataRun r2{5, 7, 0, u64(i64(100) + delta), false};
        a.runs() = {r1, r2};
        std::vector<u8> enc = a.encode();
        RunList b;
        ASSERT_EQ(RunList::decode(enc.data(), u32(enc.size()), 0, 1 << 20, b), OK)
            << "delta " << delta;
        ASSERT_EQ(b.runs().size(), 2u) << "delta " << delta;
        EXPECT_EQ(b.runs()[1].lcn, u64(i64(100) + delta)) << "delta " << delta;
    }
}

TEST(RunList, ExtentCoalesce) {
    RunList a;
    DataRun r1{0, 0x10, 0, 0x1000, false};
    DataRun r2{0x10, 0x10, 0, 0x1010, false}; /* contiguous */
    DataRun r3{0x20, 0x10, 0, 0x5000, false}; /* gap */
    a.runs() = {r1, r2, r3};
    std::vector<RunList::Extent> ex;
    a.extents(ex);
    ASSERT_EQ(ex.size(), 2u);
    EXPECT_EQ(ex[0].lcn, 0x1000u);
    EXPECT_EQ(ex[0].clusters, 0x20u);
    EXPECT_EQ(ex[1].lcn, 0x5000u);
}
