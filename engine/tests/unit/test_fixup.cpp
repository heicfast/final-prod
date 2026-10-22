/* USA fixup validation tests - corpus ground-truth section 3. */
#include <gtest/gtest.h>
#include "cr/ntfs/cr_fixup.h"
#include <cstring>
#include <vector>

using namespace cr;
using cr::ntfs::fixup_apply;
using cr::ntfs::fixup_validate;

namespace {
/* Build a synthetic 4-sector record with a correct USA. */
struct SynthRecord {
    std::vector<u8> data;
    u32 usa_off;
    u32 usa_count;
    u32 sector;
    SynthRecord(u32 sector_size = 512, u8 fill = 0xAB) {
        const u32 sectors = 4;
        data.assign(u64(sectors) * sector_size, fill);
        /* place USA at offset 0x30 (typical), record header area zeroed */
        usa_off = 0x30;
        usa_count = sectors + 1;
        u16 usn = 0x1234;
        /* slot 0 = USN; slots 1..4 = original tail values */
        std::vector<u16> slots{usn, 0x1111, 0x2222, 0x3333, 0x4444};
        for (u32 i = 0; i < usa_count; ++i) {
            data[usa_off + i * 2] = u8(slots[i]);
            data[usa_off + i * 2 + 1] = u8(slots[i] >> 8);
        }
        /* set each sector's tail u16 to the USN (as on-disk torn state) */
        for (u32 i = 0; i < sectors; ++i) {
            u8 *tail = data.data() + u64(i) * sector_size + sector_size - 2;
            tail[0] = u8(usn); tail[1] = u8(usn >> 8);
        }
        sector = sector_size;
    }
};
} // namespace

TEST(Fixup, ValidRecordApplies) {
    SynthRecord r;
    ASSERT_EQ(fixup_validate(r.data.data(), r.data.size(), r.usa_off, r.usa_count, r.sector),
              OK);
    ASSERT_EQ(fixup_apply(r.data.data(), r.data.size(), r.usa_off, r.usa_count, r.sector),
              OK);
    /* after apply, sector tails hold the originals */
    EXPECT_EQ(cr::u16_from_bytes_le(r.data.data() + 510), 0x1111);
    EXPECT_EQ(cr::u16_from_bytes_le(r.data.data() + 1022), 0x2222);
    EXPECT_EQ(cr::u16_from_bytes_le(r.data.data() + 1534), 0x3333);
    EXPECT_EQ(cr::u16_from_bytes_le(r.data.data() + 2046), 0x4444);
}

TEST(Fixup, TornRecordRejected) {
    SynthRecord r;
    /* corrupt sector 2's tail */
    r.data[2 * 512 + 510] = 0x00;
    EXPECT_EQ(fixup_validate(r.data.data(), r.data.size(), r.usa_off, r.usa_count, r.sector),
              ERR_FIXUP_FAILED);
    /* apply must refuse without touching the buffer */
    std::vector<u8> before = r.data;
    EXPECT_EQ(fixup_apply(r.data.data(), r.data.size(), r.usa_off, r.usa_count, r.sector),
              ERR_FIXUP_FAILED);
    EXPECT_EQ(std::memcmp(before.data(), r.data.data(), r.data.size()), 0);
}

TEST(Fixup, MinSizeGate) {
    SynthRecord r;
    /* below 0x400 - corpus gate */
    EXPECT_EQ(fixup_validate(r.data.data(), 0x3FF, r.usa_off, r.usa_count, r.sector),
              ERR_BAD_PARAMETER);
}

TEST(Fixup, ZeroOffsetRejected) {
    SynthRecord r;
    EXPECT_EQ(fixup_validate(r.data.data(), r.data.size(), 0, r.usa_count, r.sector),
              ERR_BAD_PARAMETER);
    /* odd offset rejected (u16 alignment) */
    EXPECT_EQ(fixup_validate(r.data.data(), r.data.size(), 0x31, r.usa_count, r.sector),
              ERR_BAD_PARAMETER);
}

TEST(Fixup, FourKnSectorSize) {
    /* 4Kn record: 4 sectors of 4096 - F1 fix honored */
    SynthRecord r(4096);
    ASSERT_EQ(fixup_validate(r.data.data(), r.data.size(), r.usa_off, r.usa_count, r.sector),
              OK);
    ASSERT_EQ(fixup_apply(r.data.data(), r.data.size(), r.usa_off, r.usa_count, r.sector),
              OK);
    EXPECT_EQ(cr::u16_from_bytes_le(r.data.data() + 1 * 4096 - 2), 0x1111);
    EXPECT_EQ(cr::u16_from_bytes_le(r.data.data() + 4 * 4096 - 2), 0x4444);
}
