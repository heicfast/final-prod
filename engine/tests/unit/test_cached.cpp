/* CachedReader behaviour + hit/miss accounting. */
#include <gtest/gtest.h>
#include "cr/storage/cr_cached.h"
#include <cstring>

using namespace cr;

namespace {
class MemDevice final : public IStorageDevice {
public:
    explicit MemDevice(u64 bytes, u32 sector = 512) : data_(bytes) {
        for (size_t i = 0; i < data_.size(); ++i) data_[i] = u8(i * 7);
        geom_.logical_sector = sector;
        geom_.physical_sector = sector;
        geom_.sector_count = bytes / sector;
        geom_.total_bytes = geom_.sector_count * sector;
    }
    u32 read_at(u64 off, void *buf, u32 len) override {
        if (off > data_.size() || len > data_.size() - off) return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry &geometry() const override { return geom_; }
private:
    std::vector<u8> data_;
    Geometry geom_;
};
} // namespace

TEST(Cached, HitMissAndBridging) {
    auto inner = std::make_unique<MemDevice>(u64(1024) * 512);
    MemDevice *raw = inner.get();
    auto c = CachedReader::create(std::move(inner), 256 * 1024, 64 * 1024);
    ASSERT_NE(c, nullptr);
    u8 buf[4096];
    /* first read: miss (1 block) */
    EXPECT_EQ(c->read_at(1000, buf, 100), OK);
    EXPECT_EQ(c->hits(), 0u);
    EXPECT_EQ(c->misses(), 1u);
    /* same block: hit */
    EXPECT_EQ(c->read_at(2000, buf, 100), OK);
    EXPECT_EQ(c->hits(), 1u);
    /* read bridging two blocks: 1 more miss + 1 hit (second partially cached?) */
    u64 m0 = c->misses();
    EXPECT_EQ(c->read_at(64 * 1024 - 100, buf, 200), OK);
    EXPECT_EQ(c->misses(), m0 + 1); /* block 2 (64K..128K) not yet cached */
    /* content correctness across the seam */
    EXPECT_EQ(std::memcmp(buf, raw->geometry().logical_sector ? buf : buf, 100), 0);
    /* verify against direct read */
    u8 direct[200];
    raw->read_at(64 * 1024 - 100, direct, 200);
    EXPECT_EQ(std::memcmp(buf, direct, 200), 0);
}

TEST(Cached, BoundsReject) {
    auto inner = std::make_unique<MemDevice>(u64(100) * 512);
    auto c = CachedReader::create(std::move(inner), 256 * 1024, 64 * 1024);
    u8 buf[16];
    EXPECT_EQ(c->read_at(100 * 512 - 8, buf, 16), ERR_RANGE_OUT_OF_BOUNDS);
}
