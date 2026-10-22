/* test_retry.cpp - RetryingDevice: retry, bisection isolation, zero-fill,
 * error-map recording.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/storage/cr_retry.h"
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

} // namespace cr
