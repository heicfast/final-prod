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
            u8* tail = data.data() + u64(i) * sector_size + sector_size - 2;
            tail[0] = u8(usn);
            tail[1] = u8(usn >> 8);
        }
        sector = sector_size;
    }
};
} // namespace

TEST(Fixup, ValidRecordApplies) {
    SynthRecord r;
    ASSERT_EQ(fixup_validate(r.data.data(), r.data.size(), r.usa_off, r.usa_count, r.sector), OK);
    ASSERT_EQ(fixup_apply(r.data.data(), r.data.size(), r.usa_off, r.usa_count, r.sector), OK);
    /* after apply, sector tails hold the originals */
    EXPECT_EQ(cr::u16_from_bytes_le(r.data.data() + 510), 0x1111);
    EXPECT_EQ(cr::u16_from_bytes_le(r.data.data() + 1022), 0x2222);
    EXPECT_EQ(cr::u16_from_bytes_le(r.data.data() + 1534), 0x3333);
    EXPECT_EQ(cr::u16_from_bytes_le(r.data.data() + 2046), 0x4444);
}

TEST(Fixup, TornRecordRejected) {
    SynthRecord r;
    /* corrupt sector 1's tail — inside the og's checked window (first two
     * tails, FUN_1800411f0 'local_18 < 2') */
    r.data[1 * 512 + 510] = 0x00;
    EXPECT_EQ(fixup_validate(r.data.data(), r.data.size(), r.usa_off, r.usa_count, r.sector),
              ERR_FIXUP_FAILED);
    /* apply must refuse without touching the buffer */
    std::vector<u8> before = r.data;
    EXPECT_EQ(fixup_apply(r.data.data(), r.data.size(), r.usa_off, r.usa_count, r.sector),
              ERR_FIXUP_FAILED);
    EXPECT_EQ(std::memcmp(before.data(), r.data.data(), r.data.size()), 0);
}

TEST(Fixup, TornThirdTailStillSalvaged) {
    /* PARITY-AUDIT F-A1-3: og FUN_1800411f0 validates only the FIRST TWO
     * sector tails ('local_18 < 2', decompiled line 52129) — a record with
     * a mismatching 3rd/4th tail is still accepted (salvage over
     * strictness; identical outcome for 1 KiB records, matters for >= 2 KiB
     * records). apply() still restores ALL tails (FUN_180041290 walks all
     * sectors). */
    SynthRecord r;                /* 4 sectors of 512 = 2 KiB record */
    r.data[2 * 512 + 510] = 0x00; /* corrupt the THIRD tail */
    r.data[3 * 512 + 511] = 0xFF; /* and the fourth */
    EXPECT_EQ(fixup_validate(r.data.data(), r.data.size(), r.usa_off, r.usa_count, r.sector), OK);
    ASSERT_EQ(fixup_apply(r.data.data(), r.data.size(), r.usa_off, r.usa_count, r.sector), OK);
    /* all four tails restored to their USA-recorded originals */
    EXPECT_EQ(cr::u16_from_bytes_le(r.data.data() + 510), 0x1111);
    EXPECT_EQ(cr::u16_from_bytes_le(r.data.data() + 1022), 0x2222);
    EXPECT_EQ(cr::u16_from_bytes_le(r.data.data() + 1534), 0x3333);
    EXPECT_EQ(cr::u16_from_bytes_le(r.data.data() + 2046), 0x4444);
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
    ASSERT_EQ(fixup_validate(r.data.data(), r.data.size(), r.usa_off, r.usa_count, r.sector), OK);
    ASSERT_EQ(fixup_apply(r.data.data(), r.data.size(), r.usa_off, r.usa_count, r.sector), OK);
    EXPECT_EQ(cr::u16_from_bytes_le(r.data.data() + 1 * 4096 - 2), 0x1111);
    EXPECT_EQ(cr::u16_from_bytes_le(r.data.data() + 4 * 4096 - 2), 0x4444);
}

/* ---- N-213: the og's record-header pre-gate band (FUN_180046160) ---- */

TEST(Fixup, OffsetBandRejectsLowAndHigh) {
    /* the og's pre-gate: usa_offset must lie in [0x2A, 0x100]
     * (:55208-55216: < 0x2A fails, < 0x101 passes, else fails).
     * RED pre-N-213: 0x08 (nonzero, even) and 0x1A0 both passed. */
    SynthRecord low;
    low.usa_off = 0x08;
    EXPECT_EQ(fixup_validate(low.data.data(), low.data.size(), low.usa_off, low.usa_count,
                             low.sector),
              ERR_BAD_PARAMETER)
        << "usa_offset below the og's 0x2A floor must reject";
    SynthRecord high;
    high.usa_off = 0x1A0;
    EXPECT_EQ(fixup_validate(high.data.data(), high.data.size(), high.usa_off, high.usa_count,
                             high.sector),
              ERR_BAD_PARAMETER)
        << "usa_offset above the og's 0x100 ceiling must reject";
}

TEST(Fixup, OffsetBandAcceptsRealVolumeOffsets) {
    /* the two offsets real volumes carry - 0x30 (1 KiB-stride records)
     * and 0x48 (4 KiB-stride records) - sit comfortably inside the band.
     * The USA array is (re)written at the offset under test so the USN
     * compare sees the record's own array, not the constructor's 0x30
     * default placement. */
    auto rewrite_usa = [](SynthRecord& r, u32 new_off) {
        const u32 cnt = r.usa_count;
        for (u32 i = 0; i < cnt; ++i) {
            const u16 v = i == 0 ? 0x1234 : u16(0x1111 + i);
            r.data[new_off + i * 2] = u8(v);
            r.data[new_off + i * 2 + 1] = u8(v >> 8);
        }
        r.usa_off = new_off;
    };
    SynthRecord r1k;
    rewrite_usa(r1k, 0x30);
    EXPECT_EQ(fixup_validate(r1k.data.data(), r1k.data.size(), r1k.usa_off, r1k.usa_count,
                             r1k.sector),
              OK);
    SynthRecord r4k(1024);
    rewrite_usa(r4k, 0x48);
    EXPECT_EQ(fixup_validate(r4k.data.data(), r4k.data.size(), r4k.usa_off, r4k.usa_count,
                             r4k.sector),
              OK);
}

TEST(Fixup, UsaCountAboveSixteenRejects) {
    /* the og's pre-gate count half (:55191-55193: count >= 0x11 fails) */
    SynthRecord r;
    r.usa_count = 0x11;
    EXPECT_EQ(fixup_validate(r.data.data(), r.data.size(), r.usa_off, r.usa_count, r.sector),
              ERR_BAD_PARAMETER)
        << "usa_count above the og's 0x10 cap must reject";
}

TEST(Fixup, IndxFloorIsTwoBytesLower) {
    /* the og's INDX sanity band (FUN_180045510 :54733-54742): the floor
     * is 0x28 (the INDX header's shorter tail), two bytes below the FILE
     * floor - a block at exactly 0x28 passes, 0x26 rejects. The family
     * is keyed on the buffer's own magic, the way the og dispatches its
     * two validators. */
    auto rewrite_usa = [](SynthRecord& r, u32 new_off) {
        for (u32 i = 0; i < r.usa_count; ++i) {
            const u16 v = i == 0 ? 0x1234 : u16(0x1111 + i);
            r.data[new_off + i * 2] = u8(v);
            r.data[new_off + i * 2 + 1] = u8(v >> 8);
        }
        r.usa_off = new_off;
    };
    SynthRecord indx;
    std::memcpy(indx.data.data(), "INDX", 4);
    rewrite_usa(indx, 0x28);
    EXPECT_EQ(fixup_validate(indx.data.data(), indx.data.size(), indx.usa_off, indx.usa_count,
                             indx.sector),
              OK)
        << "INDX blocks carry usa_offset 0x28 - inside their band";
    SynthRecord indx_low;
    std::memcpy(indx_low.data.data(), "INDX", 4);
    indx_low.usa_off = 0x26;
    EXPECT_EQ(fixup_validate(indx_low.data.data(), indx_low.data.size(), indx_low.usa_off,
                             indx_low.usa_count, indx_low.sector),
              ERR_BAD_PARAMETER)
        << "usa_offset below the INDX floor must reject";
    /* the FILE floor is stricter than the INDX floor: a FILE-family
     * buffer at 0x28 (below 0x2A) rejects (FUN_180046160 :55208) */
    SynthRecord file_low;
    file_low.usa_off = 0x28;
    EXPECT_EQ(fixup_validate(file_low.data.data(), file_low.data.size(), file_low.usa_off,
                             file_low.usa_count, file_low.sector),
              ERR_BAD_PARAMETER)
        << "FILE records reject 0x28 - their floor is 0x2A";
}
