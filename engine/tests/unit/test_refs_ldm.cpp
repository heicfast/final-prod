/* test_refs_ldm.cpp - ReFS detection + LDM recognition.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/refs/cr_refs_detect.h"
#include "cr/table/cr_ldm.h"
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

void put16be(u8* p, u16 v) {
    p[0] = u8(v >> 8);
    p[1] = u8(v);
}
void put32be(u8* p, u32 v) {
    p[0] = u8(v >> 24);
    p[1] = u8(v >> 16);
    p[2] = u8(v >> 8);
    p[3] = u8(v);
}
void put64be(u8* p, u64 v) {
    put32be(p, u32(v >> 32));
    put32be(p + 4, u32(v));
}

TEST(Ldm, FindsDatabase) {
    /* 8 MiB disk, LDM database in the LAST MiB */
    const u64 IMG = 8ull << 20;
    std::vector<u8> d(size_t(IMG), 0);
    const u64 base = IMG - (1ull << 20);
    /* TOCBLOCK at +0 of the private region copy */
    u8* toc = d.data() + base;
    memcpy(toc, "TOCBLOCK", 8);
    memcpy(toc + 0x24, "config", 6);
    memcpy(toc + 0x46, "log", 3);
    /* VMDB after the TOCBLOCK header area */
    u8* vmdb = toc + 0xC00;
    memcpy(vmdb, "VMDB", 4);
    put32be(vmdb + 0x08, 100);                   /* vblk_count */
    put32be(vmdb + 0x0C, 0x11);                  /* vblk_offset */
    put32be(vmdb + 0x10, 128);                   /* vblk_size */
    put16be(vmdb + 0x12, 4);                     /* version major */
    put16be(vmdb + 0x14, 10);                    /* version minor */
    put16be(vmdb + 0x10 + 0, 1);                 /* (size field region) */
    put64be(vmdb + 0x75, 0x1122334455667788ULL); /* disk group id */

    MemDevice dev(d);
    table::LdmInfo info;
    EXPECT_EQ(OK, table::parse_ldm_database(&dev, info));
    EXPECT_TRUE(info.found);
    EXPECT_TRUE(info.tocblock_ok);
    EXPECT_EQ(u16(4), info.vmdb_version_major);
    EXPECT_EQ(u16(10), info.vmdb_version_minor);
    EXPECT_EQ(u64(0x1122334455667788ULL), info.disk_group_id);
    EXPECT_EQ(u64(base), info.db_offset);
}

TEST(Ldm, NonLdmRejected) {
    std::vector<u8> d(8ull << 20, 0x55);
    MemDevice dev(d);
    table::LdmInfo info;
    EXPECT_EQ(ERR_BAD_MAGIC, table::parse_ldm_database(&dev, info));
    EXPECT_FALSE(info.found);
}

} // namespace

} // namespace cr
