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
        for (size_t i = 0; i < data_.size(); ++i)
            data_[i] = u8(i);
        geom_.logical_sector = sector;
        geom_.physical_sector = sector;
        geom_.sector_count = bytes / sector;
        geom_.total_bytes = geom_.sector_count * sector;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }

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
    EXPECT_EQ(buf[0], u8((10 * 512) & 0xFF));
    /* read crossing the end - refused. M-032/D-74: the og's virtual
     * volume CLAMPS + zero-fills tail-crossing reads (HFSPlusSearchDLL
     * FUN_180009290 :7219-7264: in-bounds prefix + memset tail, call
     * succeeds); ours refuses the whole read - the registered deliberate
     * divergence. The engine audit found no reliance on the og contract
     * (the HFS+ journal ring reads are bounded within the ring; every
     * other engine reads with explicitly bounded lengths and degrades
     * gracefully on refusal). The og's tail memset is in SECTOR units
     * (count-n, :7258) - an under-zeroing defect NOT adopted here. */
    EXPECT_EQ(slice->read_at(20 * 512 - 16, buf, 32), ERR_RANGE_OUT_OF_BOUNDS);
    /* read fully outside */
    EXPECT_EQ(slice->read_at(20 * 512 + 1, buf, 512), ERR_RANGE_OUT_OF_BOUNDS);
    /* read at very end edge - fine */
    EXPECT_EQ(slice->read_at(20 * 512 - 512, buf, 512), OK);
}

/* M-032 (diff-to-fix), decision-gate outcome pinned: the slice layer's
 * tail-crossing refusal is a DELIBERATE divergence from the og's
 * clamp+zero-fill virtual-volume contract (corpus
 * HFSPlusSearchDLL_decompiled.c FUN_180009290 :7219-7264, evidence
 * s3-virtual-volume-device.md section 2). Engine audit this session: no
 * engine issues crossing reads relying on zero-fill (journal_ring_read
 * bounds every take within the ring; all other reads are explicitly
 * bounded; refusals degrade through the damage counters). Registered as
 * PARITY-DELTAS D-74 - the safer contract stays. */
TEST(Slice, TailReadRefusalPinned) {
    auto dev = std::make_unique<MemDevice>(u64(100) * 512);
    u32 err = 0;
    auto slice = PartitionSlice::create(dev.get(), 10 * 512, 20 * 512, &err);
    ASSERT_NE(slice, nullptr);
    EXPECT_EQ(err, OK);
    u8 buf[512] = {0};
    std::memset(buf, 0xAA, sizeof(buf));
    /* the falsifiable claim from the finding: under the og semantics this
     * succeeds with 512 zeros past the end; ours refuses */
    EXPECT_EQ(slice->read_at(20 * 512 - 16, buf, 32), ERR_RANGE_OUT_OF_BOUNDS);
    EXPECT_EQ(buf[0], u8(0xAA)) << "a refused read never touches the buffer";
    /* prefix-only reads still succeed */
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
    m.add(100, 50, 3);  /* [100,150) */
    m.add(200, 10, 2);  /* [200,210) */
    m.add(145, 100, 5); /* [145,245): bridges BOTH -> single [100,245) */
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
