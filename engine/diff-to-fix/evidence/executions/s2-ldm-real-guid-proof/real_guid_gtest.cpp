// s2 proof: Ldm.GptDynamicDiskCanonicalGuidsParse with REAL canonical Microsoft
// LDM type GUIDs (PARTITION_LDM_METADATA_GUID 5808c8aa-7e8f-42e0-85d2-e1e90434cfb3,
// PARTITION_LDM_DATA_GUID af9b60a0-1431-4f62-bc68-3311714a69ad — Microsoft Learn
// PARTITION_INFORMATION_GPT, Linux kernel efi.h, and the og binaries themselves).
// Body copied verbatim from tests/unit/test_ldm.cpp:519 GptDynamicDiskCanonicalGuidsParse,
// with ONLY the two entry GUIDs changed to the real canonical disk bytes.
#include "cr/table/cr_ldm.h"
#include "cr/table/cr_table_wire.h"
#include "cr/storage/cr_device.h"
#include <gtest/gtest.h>
#include <cstring>
#include <vector>
#include "test_fixtures.hpp"

using namespace cr;
using namespace cr::table;
using cr::testfixture::StripeDisk;
using cr::testfixture::LdmFixture;

/* MemDevice copied from tests/unit/test_ldm.cpp:31-50 (private to that TU) */
class MemDevice : public cr::IStorageDevice {
public:
    explicit MemDevice(std::vector<cr::u8> d) : d_(std::move(d)) {
        g_.logical_sector = 512;
        g_.physical_sector = 512;
        g_.total_bytes = d_.size();
        g_.sector_count = d_.size() / 512;
    }
    cr::u32 read_at(cr::u64 off, void* buf, cr::u32 len) override {
        if (off > d_.size() || len > d_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        memcpy(buf, d_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return g_; }
private:
    Geometry g_{};
    std::vector<cr::u8> d_;
};

static void put32le(cr::u8* p, cr::u32 v) { p[0]=cr::u8(v); p[1]=cr::u8(v>>8); p[2]=cr::u8(v>>16); p[3]=cr::u8(v>>24); }
static void put64le(cr::u8* p, cr::u64 v) { for (int i=0;i<8;i++) p[i]=cr::u8(v>>(8*i)); }
static void p32be(cr::u8* p, cr::u32 v) { p[0]=cr::u8(v>>24); p[1]=cr::u8(v>>16); p[2]=cr::u8(v>>8); p[3]=cr::u8(v); }

TEST(Ldm, GptDynamicDiskRealCanonicalGuidsParse) {
    StripeDisk sd;
    const cr::u64 total_secs = 65536;
    const cr::u64 cfg_start = total_secs - 2048;
    const cr::u64 ldm_meta_last = 65532;
    sd.fx.write_privhead(ldm_meta_last);
    std::vector<cr::u8> disk = sd.fx.take();

    cr::u8* mbr = disk.data();
    mbr[0x1BE + 4] = 0xEE;
    p32be(mbr + 0x1BE + 8, 1);
    p32be(mbr + 0x1BE + 12, cr::u32(total_secs - 1));

    cr::u8* h = disk.data() + 512;
    std::memcpy(h, "EFI PART", 8);
    put32le(h + 8, 0x00010000);
    put32le(h + 12, 92);
    put64le(h + 24, 1);
    put64le(h + 32, 65535);
    put64le(h + 40, 2);
    put64le(h + 48, 65531);
    put64le(h + 72, 2);
    put32le(h + 80, 2);
    put32le(h + 84, 128);

    /* THE ONLY CHANGE from GptDynamicDiskCanonicalGuidsParse:
     * the two entry GUIDs are the REAL canonical Microsoft bytes
     * (mixed-endian disk form), not the core's GPT_TYPE_* constants. */
    const cr::u8 REAL_META[16] = {0xAA,0xC8,0x08,0x58,0x8F,0x7E,0xE0,0x42,0x85,0xD2,0xE1,0xE9,0x04,0x34,0xCF,0xB3};
    const cr::u8 REAL_DATA[16] = {0xA0,0x60,0x9B,0xAF,0x31,0x14,0x62,0x4F,0xBC,0x68,0x33,0x11,0x71,0x4A,0x69,0xAD};
    std::vector<cr::u8> entries(2 * 128, 0);
    std::memcpy(entries.data() + 0, REAL_META, 16);
    put64le(entries.data() + 32, cfg_start);
    put64le(entries.data() + 40, ldm_meta_last);
    std::memcpy(entries.data() + 128, REAL_DATA, 16);
    put64le(entries.data() + 128 + 32, 2048);
    put64le(entries.data() + 128 + 40, 4096);
    put32le(h + 88, crc32(entries.data(), entries.size()));
    std::memcpy(disk.data() + 2 * 512, entries.data(), entries.size());
    put32le(h + 16, crc32(h, 92));

    std::memcpy(disk.data() + 65534 * 512, entries.data(), entries.size());
    cr::u8* bh = disk.data() + 65535 * 512;
    std::memcpy(bh, h, 512);
    put32le(bh + 16, 0);
    put64le(bh + 24, 65535);
    put64le(bh + 32, 1);
    put64le(bh + 72, 65534);
    put32le(bh + 16, crc32(bh, 92));

    MemDevice dev(std::move(disk));
    LdmInfo info;
    const cr::u32 rc = parse_ldm_database(&dev, info);
    EXPECT_EQ(OK, rc) << "real canonical GUIDs: GPT dynamic disk MUST parse (og + Microsoft + kernel all use these bytes)";
    if (rc == OK) {
        EXPECT_TRUE(info.is_gpt);
        EXPECT_EQ(size_t(2), info.layouts.size());
    }
}
