/* test_transport_win.cpp - Win32 transport-layer contracts (Windows lane only).
 *
 * Session-97 recall-0 defect fix: the F12/N-013 destination-overlap arming
 * policy. The pre-fix callers armed the SafeWriter with the destination
 * volume's spans UNCONDITIONALLY, and the writer's per-item check then
 * compared ABSOLUTE offsets from two DIFFERENT disks numerically - the
 * benchmark runner's single-volume work disk (D:, GPT, span starting
 * ~135 MiB) numerically intersected virtually every source extent above
 * that offset, refusing 10,248 of 10,405 scenario items with
 * ERR_DEST_OVERLAP and driving scenario hash-recall to 0.0 in BOTH
 * transport eras (runs 41042267586 driver + 41140212296 direct - the
 * defect is transport-independent, the arming policy predates the
 * direct path).
 *
 * The contract under test: dest_overlap_spans() arms ONLY when an extent
 * of the destination volume actually sits on the SOURCE disk (the one
 * case where the numeric comparison is meaningful - both coordinate
 * spaces are then the same disk's). Cross-disk destinations are disjoint
 * by hardware and never arm; an unknown source disk never arms.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include <gtest/gtest.h>
#include "cr/win/cr_transport.h"

#include <utility>
#include <vector>

using namespace cr;
using namespace cr::win;

namespace {

VolExtent ext(u32 disk, u64 start, u64 len) {
    VolExtent e{};
    e.disk = disk;
    e.start = start;
    e.len = len;
    return e;
}

} // namespace

/* The runner case verbatim: dest D: on disk 1 spanning [135 MiB, ~220 GiB],
 * source the benchmark VHD on disk 3 with extents across [~1.3 GiB,
 * ~4.5 GiB] - numerically intersecting, physically disjoint. The pre-fix
 * arming refused every such item; the fix must not arm at all. */
TEST(DestOverlapSpans, CrossDiskNeverArms) {
    std::vector<VolExtent> ve;
    ve.push_back(ext(1, 135ull << 20, 219ull << 30));
    std::vector<std::pair<u64, u64>> spans;
    ASSERT_FALSE(dest_overlap_spans(ve, true, 3, &spans));
    EXPECT_TRUE(spans.empty());
}

/* Same-disk: the one meaningful case - armed, with ALL extents. */
TEST(DestOverlapSpans, SameDiskArmsWithAllSpans) {
    std::vector<VolExtent> ve;
    ve.push_back(ext(3, 135ull << 20, 1ull << 30));
    ve.push_back(ext(5, 999ull << 20, 1ull << 30)); /* spanned: other disk */
    ve.push_back(ext(3, 5ull << 30, 2ull << 30));   /* spanned: back on source */
    std::vector<std::pair<u64, u64>> spans;
    ASSERT_TRUE(dest_overlap_spans(ve, true, 3, &spans));
    ASSERT_EQ(size_t(3), spans.size()); /* N-013: every extent arms, not just
                                           the source-disk ones */
    EXPECT_EQ(135ull << 20, spans[0].first);
    EXPECT_EQ(999ull << 20, spans[1].first);
    EXPECT_EQ(5ull << 30, spans[2].first);
}

/* Unknown source disk (the image class reaches the helper only if a caller
 * gates it wrong - the safe default is to never arm). */
TEST(DestOverlapSpans, UnknownSourceDiskNeverArms) {
    std::vector<VolExtent> ve;
    ve.push_back(ext(1, 0, 1ull << 30));
    std::vector<std::pair<u64, u64>> spans;
    ASSERT_FALSE(dest_overlap_spans(ve, false, 1, &spans));
}

/* No extents / no out pointer: not armed, no crash. */
TEST(DestOverlapSpans, DegenerateInputs) {
    std::vector<VolExtent> empty;
    std::vector<std::pair<u64, u64>> spans;
    EXPECT_FALSE(dest_overlap_spans(empty, true, 1, &spans));
    std::vector<VolExtent> ve;
    ve.push_back(ext(1, 0, 4096));
    EXPECT_FALSE(dest_overlap_spans(ve, true, 1, nullptr));
}

/* The zero-offset adjacency: a destination extent STARTING at 0 on another
 * disk still never arms (offset 0 is the most collision-prone numeric value
 * - exactly the residential-data false-positive shape). */
TEST(DestOverlapSpans, ZeroOffsetCrossDiskNeverArms) {
    std::vector<VolExtent> ve;
    ve.push_back(ext(0, 0, 256ull << 30)); /* dest = whole of disk 0 */
    std::vector<std::pair<u64, u64>> spans;
    ASSERT_FALSE(dest_overlap_spans(ve, true, 2, &spans));
}
