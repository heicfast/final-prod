/* test_sat_stamps.cpp - the SAT CDB stamps and the shared SMART buffer
 * parse, byte-exact against the corpus decompile.
 *
 * Ground truth rows (DiskHealth_decompiled.c):
 *   SMART READ DATA    FUN_00419280 case 4:  local_1048 = 0xd00e08a1,
 *     local_1044 = 0xc24f0101, CDB[8]=dev, CDB[9]=0xb0
 *   SMART THRESHOLDS   FUN_0041a440 case 4:  0xd10e08a1 / 0xc24f0101 / 0xb0
 *   IDENTIFY           the :9124 sibling:    0x000e08a1 / 0x00000001 / 0xec
 *   The -1A retry      case 0xb: CDB[1] 0x1C instead of 0x08
 *   (Little-endian u32 stores: 0xd00e08a1 -> A1 08 0E D0, etc.)
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/health/cr_sat.h"
#include "cr/health/cr_smart_ata.h"
#include "cr/cr_error.h"
#include <gtest/gtest.h>
#include <cstring>

using cr::u16;
using cr::u32;
using cr::u8;
using cr::health::read_smart_ata_buffers;
using cr::health::SmartInfo;
using cr::health::sat::build_cdb12;

namespace {

bool stamps_equal(const cr::u8 got[12], const cr::u8 (&want)[12]) {
    return std::memcmp(got, want, 12) == 0;
}

} // namespace

TEST(SatStamps, SmartReadDataMatchesTheReferenceByteForByte) {
    cr::u8 cdb[12];
    ASSERT_TRUE(build_cdb12(0xD0, 0, 0xB0, 0xA0, false, cdb));
    const cr::u8 want[12] = {0xA1, 0x08, 0x0E, 0xD0, 0x01, 0x01,
                             0x4F, 0xC2, 0xA0, 0xB0, 0x00, 0x00};
    EXPECT_TRUE(stamps_equal(cdb, want));
}

TEST(SatStamps, SmartThresholdsMatchTheReferenceByteForByte) {
    cr::u8 cdb[12];
    ASSERT_TRUE(build_cdb12(0xD1, 0, 0xB0, 0xA0, false, cdb));
    const cr::u8 want[12] = {0xA1, 0x08, 0x0E, 0xD1, 0x01, 0x01,
                             0x4F, 0xC2, 0xA0, 0xB0, 0x00, 0x00};
    EXPECT_TRUE(stamps_equal(cdb, want));
}

TEST(SatStamps, IdentifyMatchesTheReferenceByteForByte) {
    cr::u8 cdb[12];
    ASSERT_TRUE(build_cdb12(0x00, 0, 0xEC, 0xA0, false, cdb));
    const cr::u8 want[12] = {0xA1, 0x08, 0x0E, 0x00, 0x01, 0x00,
                             0x00, 0x00, 0xA0, 0xEC, 0x00, 0x00};
    EXPECT_TRUE(stamps_equal(cdb, want));
}

TEST(SatStamps, TheOneARetryEncodingFlipsOnlyTheProtocolByte) {
    cr::u8 primary[12], retry[12];
    ASSERT_TRUE(build_cdb12(0xD0, 0, 0xB0, 0xA0, false, primary));
    ASSERT_TRUE(build_cdb12(0xD0, 0, 0xB0, 0xA0, true, retry));
    EXPECT_EQ(0x08, primary[1]);
    EXPECT_EQ(0x1C, retry[1]); /* FUN_00419280 case 0xb: 0xd00e1ca1 */
    for (int i = 0; i < 12; ++i) {
        if (i != 1) {
            EXPECT_EQ(primary[i], retry[i]) << "byte " << i;
        }
    }
}

TEST(SatStamps, SectorCountIsCarriedWhenIssued) {
    cr::u8 cdb[12];
    ASSERT_TRUE(build_cdb12(0xD0, 5, 0xB0, 0xA0, false, cdb));
    EXPECT_EQ(5, cdb[4]); /* a caller-issued count is honored, not forced */
}

TEST(SatStamps, UnknownCommandsAreRefusedNeverInvented) {
    cr::u8 cdb[12];
    EXPECT_FALSE(build_cdb12(0x00, 0, 0xEF, 0xA0, false, cdb)); /* no FEATURE cmd stamp */
    EXPECT_FALSE(build_cdb12(0x00, 0, 0x00, 0xA0, false, cdb)); /* zero command */
}

TEST(SatStamps, DeviceSelectIsCarried) {
    cr::u8 cdb[12];
    ASSERT_TRUE(build_cdb12(0x00, 0, 0xEC, 0xB0, false, cdb)); /* slave bit */
    EXPECT_EQ(0xB0, cdb[8]);
}

/* ---- the shared buffer parse (the WMI fallback's path) ---- */

namespace {

/* Build one 12-byte attribute entry at table offset 2 + e*12. */
void put_attr(u8* table, u32 e, u8 id, u16 flags, u8 value, u8 worst, const u8 (&raw)[6]) {
    u8* p = table + 2 + e * 12;
    std::memset(p, 0, 12);
    p[0] = id;
    p[1] = u8(flags & 0xFF);
    p[2] = u8(flags >> 8);
    p[3] = value;
    p[4] = worst;
    std::memcpy(p + 5, raw, 6);
}

} // namespace

TEST(SmartBufferParse, ParsesAttributesThresholdsAndVerdicts) {
    u8 table[512];
    std::memset(table, 0, sizeof(table));
    table[0] = 0x10; /* smart_version low byte */
    table[1] = 0x01; /* smart_version = 0x0110 */

    const u8 raw5[6] = {4, 0, 0, 0, 0, 0};       /* 4 reallocated */
    const u8 raw9[6] = {0x34, 0x12, 0, 0, 0, 0}; /* 0x1234 hours */
    const u8 raw194[6] = {31, 0, 0, 0, 0, 0};    /* 31 C */
    const u8 raw197[6] = {2, 0, 0, 0, 0, 0};     /* 2 pending */
    put_attr(table, 0, 5, 0x002F, 100, 100, raw5);
    put_attr(table, 1, 9, 0x0032, 98, 98, raw9);
    put_attr(table, 2, 10, 0x0013, 100, 100, raw5);
    put_attr(table, 3, 194, 0x0022, 65, 60, raw194);
    put_attr(table, 4, 197, 0x0012, 100, 100, raw197);
    put_attr(table, 5, 198, 0x0008, 100, 100, raw5); /* FAILED NOW bit */

    u8 thr[512];
    std::memset(thr, 0, sizeof(thr));
    for (u32 e = 0; e < 30; ++e) {
        u8* p = thr + 2 + e * 12;
        p[0] = table[2 + e * 12]; /* same id */
        p[1] = 36;                /* threshold 36 */
    }

    SmartInfo info;
    ASSERT_EQ(read_smart_ata_buffers(table, thr, "WDC WD40EFRX-68N32N0", info), u32(cr::OK));

    EXPECT_EQ(0x0110, info.smart_version);
    EXPECT_EQ("WDC WD40EFRX-68N32N0", info.model);
    ASSERT_EQ(6u, info.attributes.size());
    EXPECT_EQ(5, info.attributes[0].id);
    EXPECT_EQ(36, info.attributes[0].vendor_threshold); /* thresholds carried */
    EXPECT_TRUE(info.failing_now);                      /* attr 198 flags bit3 */
    EXPECT_EQ(4u, info.reallocated_sectors);
    EXPECT_EQ(0x1234u, info.power_on_hours);
    EXPECT_EQ(31u, info.temperature_c);
    EXPECT_EQ(2u, info.pending_sectors);
}

TEST(SmartBufferParse, MissingThresholdsAreNonFatal) {
    u8 table[512];
    std::memset(table, 0, sizeof(table));
    const u8 raw[6] = {0, 0, 0, 0, 0, 0};
    put_attr(table, 0, 194, 0x0022, 65, 60, raw);

    SmartInfo info;
    ASSERT_EQ(read_smart_ata_buffers(table, nullptr, "MODEL", info), u32(cr::OK));
    ASSERT_EQ(1u, info.attributes.size());
    EXPECT_EQ(0, info.attributes[0].vendor_threshold); /* honestly unavailable */
}

TEST(SmartBufferParse, WmiPathLeavesAtaVersionAtZero) {
    /* ata_version is IDENTIFY-only; the buffer path never touches it. */
    u8 table[512];
    std::memset(table, 0, sizeof(table));
    SmartInfo info;
    info.ata_version = 0;
    ASSERT_EQ(read_smart_ata_buffers(table, nullptr, "", info), u32(cr::OK));
    EXPECT_EQ(0, info.ata_version);
}
