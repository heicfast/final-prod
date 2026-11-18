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

#include <windows.h>

#include <atomic>
#include <cstdio>
#include <thread>
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

/* ===================== the overlapped read-ahead ring =====================
 * 2026-11-18 performance session (PERF-LEVERS F2/F10 opened): the
 * FILE_FLAG_OVERLAPPED + IOCP sliding-window transport. These tests ride
 * open_file_async() on a real temp file (the ring's documented surface):
 * byte-exactness across shapes (aligned, ragged, tail, multi-run),
 * read-ahead actually issuing ahead (latency hiding), concurrent readers
 * byte-identical, and immediate teardown with slots in flight (the
 * CancelIoEx + drain + join discipline).
 * ========================================================================== */

#include <cstdio>

namespace {
/* deterministic pattern: byte i = (i*131 + 7) & 0xFF */
unsigned char pat(u64 i) {
    return unsigned char((i * 131 + 7) & 0xFF);
}

wchar_t* make_pattern_file(u64 bytes) {
    static wchar_t path[MAX_PATH];
    wchar_t dir[MAX_PATH];
    if (GetTempPathW(MAX_PATH, dir) == 0)
        return nullptr;
    GetTempFileNameW(dir, L"ring", 0, path);
    HANDLE h =
        CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return nullptr;
    std::vector<unsigned char> buf(1 << 20);
    u64 written = 0;
    while (written < bytes) {
        const u32 chunk = u32(bytes - written < u64(buf.size()) ? bytes - written : buf.size());
        for (u32 i = 0; i < chunk; ++i)
            buf[i] = pat(written + i);
        DWORD put = 0;
        if (!WriteFile(h, buf.data(), chunk, &put, nullptr) || put != chunk) {
            CloseHandle(h);
            DeleteFileW(path);
            return nullptr;
        }
        written += chunk;
    }
    CloseHandle(h);
    return path;
}

bool check_read(cr::IStorageDevice* dev, u64 off, u32 len) {
    std::vector<unsigned char> got(len);
    if (dev->read_at(off, got.data(), len) != cr::OK)
        return false;
    for (u32 i = 0; i < len; ++i)
        if (got[i] != pat(off + i))
            return false;
    return true;
}
} // namespace

TEST(AsyncRing, ByteExactAcrossShapes) {
    wchar_t* path = make_pattern_file(24ull << 20);
    ASSERT_NE(path, nullptr);
    u32 err = 0;
    auto dev = cr::win::open_file_async(path, 0, 0, &err);
    ASSERT_NE(dev, nullptr) << "err=" << err;
    EXPECT_EQ(err, cr::OK);
    /* aligned */
    EXPECT_TRUE(check_read(dev.get(), 0, 4096));
    EXPECT_TRUE(check_read(dev.get(), 4 << 20, 256 * 1024));
    /* ragged (unaligned offset + odd length - the envelope floor/ceil) */
    EXPECT_TRUE(check_read(dev.get(), 1234, 333));
    EXPECT_TRUE(check_read(dev.get(), (8 << 20) + 65531, 9001));
    /* multi-run (bigger than one 4 MiB window) */
    EXPECT_TRUE(check_read(dev.get(), 3 << 20, 9 << 20));
    /* the device tail (documented tail clamp) */
    EXPECT_TRUE(check_read(dev.get(), (24ull << 20) - 4096, 4096));
    /* bounds contract preserved */
    unsigned char b;
    EXPECT_EQ(dev->read_at(24ull << 20, &b, 1), cr::ERR_RANGE_OUT_OF_BOUNDS);
    DeleteFileW(path);
}

TEST(AsyncRing, SequentialReadAheadServes) {
    /* a long sequential walk: every 64 KiB step must hit - by construction
     * the ring issues ahead; the assertion is the byte truth + that the
     * whole walk completes (the readahead never wedges). */
    wchar_t* path = make_pattern_file(48ull << 20);
    ASSERT_NE(path, nullptr);
    u32 err = 0;
    auto dev = cr::win::open_file_async(path, 0, 0, &err);
    ASSERT_NE(dev, nullptr);
    for (u64 off = 0; off + 4096 <= 48ull << 20; off += 64 * 1024)
        if (!check_read(dev.get(), off, 4096)) {
            FAIL() << "readahead walk broke at off=" << off;
        }
    DeleteFileW(path);
}

TEST(AsyncRing, ConcurrentReadersByteIdentical) {
    wchar_t* path = make_pattern_file(32ull << 20);
    ASSERT_NE(path, nullptr);
    u32 err = 0;
    auto dev = cr::win::open_file_async(path, 0, 0, &err);
    ASSERT_NE(dev, nullptr);
    std::atomic<int> bad{0};
    std::vector<std::thread> th;
    for (int t = 0; t < 4; ++t) {
        th.emplace_back([&, t] {
            std::vector<unsigned char> buf(8192);
            u64 st = 0x9E3779B97F4A7C15ULL * (t + 1);
            for (int k = 0; k < 4000; ++k) {
                st ^= st << 13;
                st ^= st >> 7;
                st ^= st << 17;
                const u64 off = st % ((32ull << 20) - 8192);
                if (dev->read_at(off, buf.data(), 8192) != cr::OK) {
                    bad.fetch_add(1);
                    return;
                }
                for (u32 i = 0; i < 8192; ++i)
                    if (buf[i] != pat(off + i)) {
                        bad.fetch_add(1);
                        return;
                    }
            }
        });
    }
    for (auto& x : th)
        x.join();
    EXPECT_EQ(bad.load(), 0);
    DeleteFileW(path);
}

TEST(AsyncRing, TuningOverrideAndTeardownInFlight) {
    /* the sweep seam: a non-default run/slot config serves byte-exact,
     * and destroying the device while read-ahead slots are IN FLIGHT is
     * clean (cancel + drain + join: no hang, no leak, no crash - the
     * destructor discipline the OVERLAPPED lifetime rules require). */
    wchar_t* path = make_pattern_file(32ull << 20);
    ASSERT_NE(path, nullptr);
    u32 err = 0;
    {
        auto dev = cr::win::open_file_async(path, 256 * 1024 /*run*/, 16 /*slots*/, &err);
        ASSERT_NE(dev, nullptr);
        EXPECT_TRUE(check_read(dev.get(), 5 << 20, 1 << 20));
        EXPECT_TRUE(check_read(dev.get(), 1000, 777));
        /* slots are in flight here (readahead was kicked by the walks);
         * the destructor runs immediately after this line */
    }
    {
        auto dev = cr::win::open_file_async(path, 0, 1 /*single slot*/, &err);
        ASSERT_NE(dev, nullptr);
        EXPECT_TRUE(check_read(dev.get(), 0, 32ull << 20));
    }
    DeleteFileW(path);
}
