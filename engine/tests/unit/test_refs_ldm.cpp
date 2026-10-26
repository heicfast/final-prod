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
    /* boot-sector version bytes (audit row 28: real values, not a
     * fabricated string) */
    d[0x28] = 3;
    d[0x29] = 4;
    MemDevice dev(d);
    refs::RefsInfo info;
    EXPECT_EQ(OK, refs::detect_refs(&dev, info));
    EXPECT_TRUE(info.found);
    EXPECT_STREQ("ReFS v3.4", info.version);
}

TEST(RefsDetect, PartialSignaturePageDetectedAndFlagged) {
    /* N-007: the og's discovery gate is the FIRST DWORD
     * (ReFSSearchDLL :11689-11720, `*param_2 == *(int*)pcVar4`); the
     * full signature is the ref-parser's job. A page with "SUPB" + a
     * corrupt tail is DISCOVERED (found) and flagged partial, not
     * rejected - the pre-fix probe returned ERR_BAD_MAGIC on exactly
     * this volume (fail-before by construction: the flag did not exist
     * and the page was refused). */
    std::vector<u8> d(4ull << 20, 0xEE);
    memcpy(d.data() + 4096, "SUPB", 4);
    d[4100] = 'X'; /* corrupt byte 5: the full signature can never match */
    MemDevice dev(d);
    refs::RefsInfo info;
    EXPECT_EQ(OK, refs::detect_refs(&dev, info));
    EXPECT_TRUE(info.found);
    EXPECT_EQ(u64(4096), info.msb_offset);
    EXPECT_TRUE(info.signature_partial);
    EXPECT_FALSE(info.checkpointed);
}

TEST(RefsDetect, FullSignaturePageNotFlaggedPartial) {
    /* the inverse boundary: a byte-exact page grades clean */
    std::vector<u8> d(4ull << 20, 0xEE);
    memcpy(d.data() + 4096, "MSB+ReFSFSRS", 12);
    memcpy(d.data() + 8192, "CHKPSUPBMSB+ReFSFSRS", 20);
    MemDevice dev(d);
    refs::RefsInfo info;
    EXPECT_EQ(OK, refs::detect_refs(&dev, info));
    EXPECT_TRUE(info.found);
    EXPECT_TRUE(info.checkpointed);
    EXPECT_FALSE(info.signature_partial);
    EXPECT_EQ(u64(8192), info.msb_offset); /* last hit wins (pre-N-007 order kept) */
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
