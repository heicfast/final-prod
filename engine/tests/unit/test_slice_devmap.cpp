/* PartitionSlice bounds + ReadErrorMap tests (R1.5, R1.6). */
#include <gtest/gtest.h>
#include "cr/storage/cr_slice.h"
#include <cstring>

using namespace cr;

namespace {
class MemDevice final : public IStorageDevice {
public:
    explicit MemDevice(u64 bytes, u32 sector = 512) {
        data_.resize(bytes);
        for (size_t i = 0; i < data_.size(); ++i) data_[i] = u8(i);
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

TEST(Slice, BoundsEnforced) {
    auto dev = std::make_unique<MemDevice>(u64(100) * 512);
    u32 err = 0;
    auto slice = PartitionSlice::create(dev.get(), 10 * 512, 20 * 512, &err);
    ASSERT_NE(slice, nullptr);
    EXPECT_EQ(err, OK);
    /* read at partition start */
    u8 buf[512];
    EXPECT_EQ(slice->read_at(0, buf, 512), OK);
    EXPECT_EQ(buf[0], u8(10 * 512));
    /* read crossing the end - refused */
    EXPECT_EQ(slice->read_at(20 * 512 - 16, buf, 32), ERR_RANGE_OUT_OF_BOUNDS);
    /* read fully outside */
    EXPECT_EQ(slice->read_at(20 * 512 + 1, buf, 512), ERR_RANGE_OUT_OF_BOUNDS);
    /* read at very end edge - fine */
    EXPECT_EQ(slice->read_at(20 * 512 - 512, buf, 512), OK);
}

TEST(Slice, StartMustBeAligned) {
    auto dev = std::make_unique<MemDevice>(u64(100) * 512);
    u32 err = 0;
    EXPECT_EQ(PartitionSlice::create(dev.get(), 10 * 512 + 3, 20 * 512, &err), nullptr);
    EXPECT_EQ(err, ERR_BAD_PARAMETER);
}

TEST(Slice, RaggedTailAllowed) {
    /* lost-partition candidates have unaligned tails - allowed by design */
    auto dev = std::make_unique<MemDevice>(u64(100) * 512);
    u32 err = 0;
    auto slice = PartitionSlice::create(dev.get(), 10 * 512, 20 * 512 - 100, &err);
    ASSERT_NE(slice, nullptr);
    u8 buf[16];
    EXPECT_EQ(slice->read_at(20 * 512 - 100 - 16, buf, 16), OK);
}

TEST(DevMap, AddMergeSerialize) {
    ReadErrorMap m;
    m.add(100, 50, 3);   /* [100,150) */
    m.add(200, 10, 2);   /* [200,210) */
    m.add(145, 100, 5);  /* [145,245): bridges BOTH -> single [100,245) */
    ASSERT_EQ(m.ranges().size(), 1u);
    EXPECT_EQ(m.ranges()[0].offset, 100u);
    EXPECT_EQ(m.ranges()[0].length, 145u); /* [100..245) */
    EXPECT_EQ(m.total_bytes(), 145u);

    std::vector<u8> out;
    m.serialize(out);
    u32 used = 0;
    ReadErrorMap m2 = ReadErrorMap::deserialize(out.data(), out.size(), &used);
    EXPECT_EQ(used, out.size());
    ASSERT_EQ(m2.ranges().size(), 1u);
    EXPECT_EQ(m2.ranges()[0].offset, 100u);
    EXPECT_EQ(m2.ranges()[0].length, 145u);
}
