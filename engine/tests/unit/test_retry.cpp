/* test_retry.cpp - RetryingDevice: retry, bisection isolation, zero-fill,
 * error-map recording.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/storage/cr_retry.h"
#include "cr/cr_errmod.h"
#include <gtest/gtest.h>
#include <cstring>
#include <vector>

namespace cr {
namespace {

/* In-memory device with programmable bad ranges (simulates bad sectors). */
class FlakyDevice final : public IStorageDevice {
public:
    FlakyDevice(u64 bytes, u32 sector = 512) {
        g_.logical_sector = sector;
        g_.physical_sector = sector;
        g_.total_bytes = bytes;
        g_.sector_count = bytes / sector;
        data_.resize(size_t(bytes), 0xAB);
    }

    void add_bad_range(u64 off, u64 len) { bad_.push_back({off, len}); }

    u32 read_at(u64 off, void* buf, u32 len) override {
        ++reads_;
        if (off > size() || len > size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        for (const auto& r : bad_) {
            if (off < r.first + r.second && r.first < off + len)
                return ERR_READ_FAILED;
        }
        memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return g_; }

    u32 reads() const { return reads_; }

private:
    Geometry g_{};
    std::vector<u8> data_;
    std::vector<std::pair<u64, u64>> bad_;
    u32 reads_ = 0;
};

} // namespace

TEST(Retry, CleanReadPassesThrough) {
    auto inner = std::make_unique<FlakyDevice>(1 << 20);
    auto* raw = inner.get();
    auto dev = RetryingDevice::create(std::move(inner));
    ASSERT_TRUE(dev);
    u8 buf[4096];
    EXPECT_EQ(OK, dev->read_at(0, buf, sizeof(buf)));
    EXPECT_EQ(0xAB, buf[0]);
    EXPECT_EQ(0u, dev->error_map().ranges().size());
    EXPECT_EQ(1u, raw->reads()); /* single attempt, no retries */
}

TEST(Retry, FailModeReportsExhaustedAndMapsRange) {
    auto inner = std::make_unique<FlakyDevice>(1 << 20);
    inner->add_bad_range(4096, 512); /* one bad sector in the middle */
    auto dev = RetryingDevice::create(std::move(inner), RetryPolicy{3, RetryMode::Fail});
    u8 buf[16384];
    /* The whole 16 KiB span fails; bisection isolates the 512-byte hole. */
    EXPECT_EQ(ERR_RETRY_EXHAUSTED, dev->read_at(0, buf, sizeof(buf)));
    ASSERT_EQ(1u, dev->error_map().ranges().size());
    EXPECT_EQ(4096u, dev->error_map().ranges()[0].offset);
    EXPECT_EQ(512u, dev->error_map().ranges()[0].length);
    EXPECT_EQ(3u, dev->error_map().ranges()[0].attempts);
}

TEST(Retry, SkipModeZeroFillsOnlyBadSectors) {
    auto inner = std::make_unique<FlakyDevice>(1 << 20);
    inner->add_bad_range(4096, 512);   /* sector-aligned: sector 8 */
    inner->add_bad_range(20480, 1024); /* sector-aligned: sectors 40-41 */
    auto dev = RetryingDevice::create(std::move(inner), RetryPolicy{2, RetryMode::Skip});
    u8 buf[32768];
    EXPECT_EQ(OK, dev->read_at(0, buf, sizeof(buf)));
    /* good bytes preserved */
    EXPECT_EQ(0xAB, buf[0]);
    EXPECT_EQ(0xAB, buf[4095]); /* last byte before first bad range */
    EXPECT_EQ(0xAB, buf[4608]); /* first byte after first bad range */
    EXPECT_EQ(0xAB, buf[20479]);
    EXPECT_EQ(0xAB, buf[21504]);
    /* bad bytes zeroed */
    EXPECT_EQ(0, buf[4096]);
    EXPECT_EQ(0, buf[4607]); /* last byte of first bad range */
    EXPECT_EQ(0, buf[20480]);
    EXPECT_EQ(0, buf[21503]);
    /* both ranges recorded, sector-granular */
    EXPECT_EQ(2u, dev->error_map().ranges().size());
    EXPECT_EQ(u64(512 + 1024), dev->error_map().total_bytes());
}

TEST(Retry, UnalignedBadRangeRoundsToWholeSectors) {
    /* A byte-granular hole inside one sector maps to that whole sector:
     * a torn sector can never be partially read, so the map is
     * sector-granular by design (bisection stops at sector size). */
    auto inner = std::make_unique<FlakyDevice>(1 << 20);
    inner->add_bad_range(20000, 300); /* inside sector 39 (19968..20479) */
    auto dev = RetryingDevice::create(std::move(inner), RetryPolicy{2, RetryMode::Skip});
    u8 buf[32768];
    EXPECT_EQ(OK, dev->read_at(0, buf, sizeof(buf)));
    EXPECT_EQ(u64(512), dev->error_map().total_bytes());
    EXPECT_EQ(19968u, dev->error_map().ranges()[0].offset);
}

/* N-004: fails only requests that START at fail_at (an alignment-dependent
 * transport), succeeds for any other start - the shape the CatchBuffer
 * backtrack exists for (the plain same-window retry can never succeed). */
class PickyDevice final : public IStorageDevice {
public:
    PickyDevice(u64 bytes, u64 fail_at, u32 sector = 512)
        : fail_at_(fail_at) {
        g_.logical_sector = sector;
        g_.physical_sector = sector;
        g_.total_bytes = bytes;
        g_.sector_count = bytes / sector;
        data_.resize(size_t(bytes), 0xAB);
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        ++reads_;
        if (off > size() || len > size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (off == fail_at_)
            return ERR_READ_FAILED;
        memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return g_; }
    u64 size() const override { return g_.total_bytes; }
    u32 reads() const { return reads_; }

private:
    Geometry g_{};
    std::vector<u8> data_;
    u64 fail_at_;
    u32 reads_ = 0;
};

TEST(Retry, BacktrackReadSalvagesPartialSector) {
    /* N-004 fail-before/pass-after: pre-fix (backtrack absent) this read
     * exhausts and maps a bad range; with the CatchBuffer backtrack the
     * shifted window succeeds and the bytes arrive - the range is never
     * recorded. The og's CEXT4CatchBufferDeviceProxy 1 KB-backtrack
     * salvage, at the core's designated bad-sector layer. */
    auto inner = std::make_unique<PickyDevice>(64 * 1024, 8192);
    auto* raw = inner.get();
    auto dev = RetryingDevice::create(std::move(inner), RetryPolicy{3, RetryMode::Fail});
    ASSERT_TRUE(dev);
    u8 buf[512];
    EXPECT_EQ(OK, dev->read_at(8192, buf, sizeof(buf)));
    EXPECT_EQ(0xAB, buf[0]);
    EXPECT_EQ(0xAB, buf[511]);
    EXPECT_EQ(0u, dev->error_map().ranges().size()); /* salvaged, not bad */
    /* 3 same-window attempts + exactly one shifted window [7168, 8704) */
    EXPECT_EQ(4u, raw->reads());
}

TEST(Retry, BacktrackOffReproducesLegacyExhausted) {
    /* The policy escape hatch: with backtrack disabled the pre-N-004
     * behavior is reproduced exactly (exhausted + mapped). */
    auto inner = std::make_unique<PickyDevice>(64 * 1024, 8192);
    auto dev = RetryingDevice::create(std::move(inner), RetryPolicy{3, RetryMode::Fail, false});
    u8 buf[512];
    EXPECT_EQ(ERR_RETRY_EXHAUSTED, dev->read_at(8192, buf, sizeof(buf)));
    ASSERT_EQ(1u, dev->error_map().ranges().size());
    EXPECT_EQ(8192u, dev->error_map().ranges()[0].offset);
    EXPECT_EQ(512u, dev->error_map().ranges()[0].length);
}

TEST(Retry, BacktrackStaysAlignedOn4Kn) {
    /* The step is max(0x400, sector): on 4Kn the shifted request starts
     * one full 4 KiB page back and stays aligned (the og's hardcoded
     * 0x400 would be misaligned at this geometry). */
    auto inner = std::make_unique<PickyDevice>(64 * 1024, 8192, 4096);
    auto* raw = inner.get();
    auto dev = RetryingDevice::create(std::move(inner), RetryPolicy{3, RetryMode::Fail});
    u8 buf[512];
    EXPECT_EQ(OK, dev->read_at(8192, buf, sizeof(buf)));
    /* shifted window [4096, 4096+4096+512): 3 + 1 reads */
    EXPECT_EQ(4u, raw->reads());
    EXPECT_EQ(0u, dev->error_map().ranges().size());
}

TEST(Retry, BoundsAreEnforcedBeforeInner) {
    auto inner = std::make_unique<FlakyDevice>(4096);
    auto* raw = inner.get();
    auto dev = RetryingDevice::create(std::move(inner));
    u8 buf[4096];
    EXPECT_EQ(ERR_RANGE_OUT_OF_BOUNDS, dev->read_at(4096, buf, 1));
    EXPECT_EQ(ERR_RANGE_OUT_OF_BOUNDS, dev->read_at(0, buf, 4097));
    EXPECT_EQ(ERR_BAD_PARAMETER, dev->read_at(0, nullptr, 16));
    EXPECT_EQ(0u, raw->reads());
}

TEST(Retry, DeterministicFailureNotRetried) {
    /* Out-of-bounds is deterministic: no retry loop, straight report. */
    auto inner = std::make_unique<FlakyDevice>(8192);
    auto* raw = inner.get();
    auto dev = RetryingDevice::create(std::move(inner), RetryPolicy{10, RetryMode::Fail});
    u8 buf[8192];
    EXPECT_EQ(ERR_RANGE_OUT_OF_BOUNDS, dev->read_at(0, buf, 8192 + 1));
    EXPECT_EQ(0u, raw->reads());
}

TEST(Retry, GeometryForwarded) {
    auto inner = std::make_unique<FlakyDevice>(1 << 20, 4096); /* 4Kn */
    auto dev = RetryingDevice::create(std::move(inner));
    EXPECT_TRUE(dev->geometry().is_4kn());
    EXPECT_EQ(u64(1 << 20), dev->size());
}

/* ---- M-049: the per-range module attribution (thread stamp + owner) ---- */

TEST(Retry, ThreadStampAttributedToRecordedRange) {
    /* a stamped thread's failures carry its module: the engine-side
     * subset of the og's per-module error registrars, recorded at the
     * failure site exactly like the og's error objects carry their
     * module context. */
    auto inner = std::make_unique<FlakyDevice>(1 << 20);
    inner->add_bad_range(4096, 512);
    auto dev = RetryingDevice::create(std::move(inner), RetryPolicy{3, RetryMode::Skip});
    u8 buf[16384];
    {
        errmod::Guard g(errmod::kNtfs);
        EXPECT_EQ(OK, dev->read_at(0, buf, sizeof(buf)));
    }
    ASSERT_EQ(1u, dev->error_map().ranges().size());
    EXPECT_EQ(u32(errmod::kNtfs), dev->error_map().ranges()[0].module);
    /* the stamp restored to none after the guard: an unstamped thread's
     * failure carries the owner fallback (here none - unattributed) */
    EXPECT_EQ(OK, dev->read_at(20480, buf, 4096));
}

TEST(Retry, OwnerFallbackCoversUnstampedWorkerThreads) {
    /* engine-internal worker pools read with no thread stamp; the
     * device-level owner (published by the orchestrator between
     * sub-passes) is the fallback attribution - the whole point of the
     * two-level design (cr_errmod.h). */
    auto inner = std::make_unique<FlakyDevice>(1 << 20);
    inner->add_bad_range(4096, 512);
    auto dev = RetryingDevice::create(std::move(inner), RetryPolicy{3, RetryMode::Skip});
    dev->set_owner_module(errmod::kCarve);
    /* simulate the unstamped worker: a thread that never set a stamp.
     * The test thread itself is unstamped here (no Guard active). */
    EXPECT_EQ(errmod::kNone, errmod::current());
    u8 buf[16384];
    EXPECT_EQ(OK, dev->read_at(0, buf, sizeof(buf)));
    ASSERT_EQ(1u, dev->error_map().ranges().size());
    EXPECT_EQ(u32(errmod::kCarve), dev->error_map().ranges()[0].module)
        << "the owner fallback attributed the unstamped read";
    /* the thread stamp takes precedence over the owner when set */
    inner = std::make_unique<FlakyDevice>(1 << 20);
    inner->add_bad_range(4096, 512);
    dev = RetryingDevice::create(std::move(inner), RetryPolicy{3, RetryMode::Skip});
    dev->set_owner_module(errmod::kCarve);
    {
        errmod::Guard g(errmod::kFat);
        EXPECT_EQ(OK, dev->read_at(0, buf, sizeof(buf)));
    }
    ASSERT_EQ(1u, dev->error_map().ranges().size());
    EXPECT_EQ(u32(errmod::kFat), dev->error_map().ranges()[0].module)
        << "the thread stamp beats the owner";
}

TEST(Errmod, GuardSetsAndRestoresTheThreadStamp) {
    /* pool threads are reused - an unrestored stamp would leak into the
     * next task; the Guard restores the previous stamp exactly. */
    EXPECT_EQ(errmod::kNone, errmod::current());
    {
        errmod::Guard a(errmod::kNtfs);
        EXPECT_EQ(u32(errmod::kNtfs), errmod::current());
        {
            errmod::Guard b(errmod::kCarve); /* nested: restores to NTFS */
            EXPECT_EQ(u32(errmod::kCarve), errmod::current());
        }
        EXPECT_EQ(u32(errmod::kNtfs), errmod::current());
    }
    EXPECT_EQ(errmod::kNone, errmod::current());
    /* set_thread returns the previous value (the Guard's mechanism) */
    EXPECT_EQ(u32(errmod::kNone), errmod::set_thread(errmod::kRefs));
    EXPECT_EQ(u32(errmod::kRefs), errmod::current());
    (void)errmod::set_thread(errmod::kNone);
    EXPECT_EQ(errmod::kNone, errmod::current());
}

} // namespace cr
