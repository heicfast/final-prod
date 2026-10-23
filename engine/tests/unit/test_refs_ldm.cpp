/* test_refs_ldm.cpp - ReFS detection (LDM tests live in test_ldm.cpp
 * since the session-5 full-implementation rewrite).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/refs/cr_refs_detect.h"
#include <gtest/gtest.h>
#include <cstring>

namespace cr {
namespace {

class MemDevice final : public IStorageDevice {
public:
    explicit MemDevice(std::vector<u8> d) : d_(std::move(d)) {
        g_.logical_sector = 512;
        g_.physical_sector = 512;
        g_.total_bytes = d_.size();
        g_.sector_count = d_.size() / 512;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > d_.size() || len > d_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        memcpy(buf, d_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return g_; }

private:
    Geometry g_{};
    std::vector<u8> d_;
};

TEST(RefsDetect, FindsMsbPages) {
    std::vector<u8> d(4ull << 20, 0xEE);
    memcpy(d.data() + 4096, "MSB+ReFSFSRS", 12);
    MemDevice dev(d);
    refs::RefsInfo info;
    EXPECT_EQ(OK, refs::detect_refs(&dev, info));
    EXPECT_TRUE(info.found);
    EXPECT_EQ(u64(4096), info.msb_offset);
    EXPECT_FALSE(info.checkpointed);
}

TEST(RefsDetect, CheckpointVariant) {
    std::vector<u8> d(4ull << 20, 0);
    memcpy(d.data() + 8192, "CHKPSUPBMSB+ReFSFSRS", 20);
    MemDevice dev(d);
    refs::RefsInfo info;
    EXPECT_EQ(OK, refs::detect_refs(&dev, info));
    EXPECT_TRUE(info.found);
    EXPECT_TRUE(info.checkpointed);
    EXPECT_EQ(u64(8192), info.msb_offset);
}

TEST(RefsDetect, IdentificationFallback) {
    std::vector<u8> d(4ull << 20, 0);
    memcpy(d.data() + 0x200, "ReFS", 4);
    MemDevice dev(d);
    refs::RefsInfo info;
    EXPECT_EQ(OK, refs::detect_refs(&dev, info));
    EXPECT_TRUE(info.found);
    EXPECT_STREQ("ReFS", info.version);
}

TEST(RefsDetect, NonRefsRejected) {
    std::vector<u8> d(4ull << 20, 0xAB);
    MemDevice dev(d);
    refs::RefsInfo info;
    EXPECT_EQ(ERR_BAD_MAGIC, refs::detect_refs(&dev, info));
    EXPECT_FALSE(info.found);
}

} // namespace

} // namespace cr
