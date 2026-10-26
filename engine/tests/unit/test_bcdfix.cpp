/* test_bcdfix.cpp - the BCD partition-offset rewriter (M-040 stage a).
 *
 * Proves, against a synthetic mini-hive built in the corpus's own cell
 * shape (Fixup_decompiled.c :499-599 decode, re-verified first-hand):
 *   1. The scan finds device-element cells BY SIGNATURE + OLD LBA.
 *   2. The rewrite writes (new_lba << 9) at the cell's offset field.
 *   3. Undo restores the original bytes byte-exactly.
 *   4. The consent gate: no callback = default-DENY, nothing written.
 *   5. Boundaries: foreign signature / foreign LBA / truncated cell /
 *      marker-adjacent noise never plan a rewrite.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include <gtest/gtest.h>
#include "cr/bcdfix/cr_bcdfix.h"
#include "cr/cr_error.h"
#include <cstring>
#include <vector>

using namespace cr;
using namespace cr::bcdfix;

namespace {

void put64(std::vector<u8>& b, size_t at, u64 v) {
    for (int i = 0; i < 8; ++i)
        b[at + size_t(i)] = u8((v >> (8 * i)) & 0xFF);
}

void put32(std::vector<u8>& b, size_t at, u32 v) {
    for (int i = 0; i < 4; ++i)
        b[at + size_t(i)] = u8((v >> (8 * i)) & 0xFF);
}

/* Build a hive with one device-element cell for (sig, start_lba) at
 * `cell_at`, plus optional noise bytes around it. */
std::vector<u8> hive_with_cell(size_t cell_at, u32 sig, u64 start_lba, size_t total = 4096) {
    std::vector<u8> hive(total, 0);
    put64(hive, cell_at, kCellMarker);
    put64(hive, cell_at + kOffsetFieldInCell, start_lba << 9);
    put32(hive, cell_at + kSignatureFieldInCell, sig);
    return hive;
}

/* A recording writer over an in-memory hive. */
struct MemHive {
    std::vector<u8> bytes;
    int writes = 0;
    std::vector<u64> offsets;
    static u32 thunk(u64 at, const u8 bytes[8], void* user) {
        MemHive* self = static_cast<MemHive*>(user);
        if (at + 8 > self->bytes.size())
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(self->bytes.data() + at, bytes, 8);
        ++self->writes;
        self->offsets.push_back(at);
        return OK;
    }
};

bool allow(void*) { return true; }
bool deny(void*) { return false; }

} // namespace

TEST(BcdFix, BcdFindsBootPartitionBySignature) {
    /* the finding's named test #2: the cell is located by MBR signature
     * + old start LBA inside the 0xffffffa0 device-element cell */
    const u32 sig = 0xA1B2C3D4;
    auto hive = hive_with_cell(512, sig, 2048);
    /* noise: same marker, same LBA, WRONG signature */
    auto noise = hive_with_cell(512 + kCellSize, 0xDEADBEEF, 2048);
    std::memcpy(hive.data() + 512 + kCellSize, noise.data() + 512 + kCellSize, kCellSize);
    /* noise: same marker, same signature, WRONG LBA */
    put64(hive, 512 + 2 * kCellSize, kCellMarker);
    put64(hive, 512 + 2 * kCellSize + kOffsetFieldInCell, 4096 << 9);
    put32(hive, 512 + 2 * kCellSize + kSignatureFieldInCell, sig);

    const auto plan = plan_rewrites(hive.data(), hive.size(), sig, 2048, 9999);
    ASSERT_EQ(plan.size(), 1u) << "exactly the one matching cell";
    EXPECT_EQ(plan[0].field_offset, u64(512 + kOffsetFieldInCell));
    /* the undo record carries the ORIGINAL bytes (2048 << 9) */
    u64 old_v = 0;
    for (int i = 7; i >= 0; --i)
        old_v = (old_v << 8) | plan[0].old_bytes[i];
    EXPECT_EQ(old_v, 2048ull << 9);
}

TEST(BcdFix, BcdOffsetRewriteWritesNewLba) {
    /* the finding's named test #1: the rewrite writes (new_lba << 9) at
     * the cell's offset field - the FUN_180002520 WriteFile shape */
    auto hive = hive_with_cell(1024, 0x11223344, 2048);
    auto plan = plan_rewrites(hive.data(), hive.size(), 0x11223344, 2048, 123456);
    ASSERT_EQ(plan.size(), 1u);

    MemHive mem{hive, 0, {}};
    size_t applied = 0;
    ASSERT_EQ(OK, execute_plan(plan, &MemHive::thunk, &mem, allow, nullptr, &applied));
    EXPECT_EQ(applied, 1u);
    EXPECT_EQ(mem.writes, 1);
    u64 v = 0;
    for (int i = 7; i >= 0; --i)
        v = (v << 8) | mem.bytes[1024 + kOffsetFieldInCell + size_t(i)];
    EXPECT_EQ(v, 123456ull << 9) << "the new LBA, stored as bytes (lba << 9)";

    /* re-scan with the NEW position finds the cell again (idempotent
     * re-plan after a move) */
    const auto replan = plan_rewrites(mem.bytes.data(), mem.bytes.size(), 0x11223344, 123456, 1);
    EXPECT_EQ(replan.size(), 1u);
}

TEST(BcdFix, BcdUndoRestoresOffsets) {
    /* the finding's named test #3: undo restores the original offsets
     * byte-exactly (the Bcd_RestoreBcdFile analog) */
    auto hive = hive_with_cell(256, 0xCAFEF00D, 8192);
    put64(hive, 256 + kCellSize + 8, kCellMarker); /* second matching cell */
    put64(hive, 256 + kCellSize + 8 + kOffsetFieldInCell, 8192 << 9);
    put32(hive, 256 + kCellSize + 8 + kSignatureFieldInCell, 0xCAFEF00D);

    auto plan = plan_rewrites(hive.data(), hive.size(), 0xCAFEF00D, 8192, 555);
    ASSERT_EQ(plan.size(), 2u);
    MemHive mem{hive, 0, {}};
    ASSERT_EQ(OK, execute_plan(plan, &MemHive::thunk, &mem, allow, nullptr));

    /* undo: every original byte back, reverse order */
    size_t undone = 0;
    ASSERT_EQ(OK, undo_plan(plan, &MemHive::thunk, &mem, &undone));
    EXPECT_EQ(undone, 2u);
    EXPECT_EQ(0, std::memcmp(mem.bytes.data(), hive.data(), hive.size()))
        << "the hive is byte-identical to the original";
}

TEST(BcdFix, BootfixRefusedWithoutConsent) {
    /* the finding's named test #4: the consent gate - no callback =
     * default-DENY (nothing written); an explicit deny = same; only an
     * explicit allow writes (P62, the rawfixer S-008 model) */
    auto hive = hive_with_cell(512, 0x0BAD0BAD, 4096);
    auto plan = plan_rewrites(hive.data(), hive.size(), 0x0BAD0BAD, 4096, 777);
    ASSERT_EQ(plan.size(), 1u);

    MemHive mem{hive, 0, {}};
    size_t applied = 99;
    EXPECT_EQ(ERR_REPAIR_REFUSED, execute_plan(plan, &MemHive::thunk, &mem, nullptr, nullptr, &applied));
    EXPECT_EQ(applied, 0u);
    EXPECT_EQ(mem.writes, 0) << "no consent callback: nothing is ever written";

    EXPECT_EQ(ERR_REPAIR_REFUSED, execute_plan(plan, &MemHive::thunk, &mem, deny, nullptr, &applied));
    EXPECT_EQ(mem.writes, 0) << "an explicit deny: nothing is written";

    EXPECT_EQ(OK, execute_plan(plan, &MemHive::thunk, &mem, allow, nullptr, &applied));
    EXPECT_EQ(mem.writes, 1) << "the explicit allow is the only write path";
}

TEST(BcdFix, BoundariesNeverPlan) {
    /* truncated buffer (cell would cross the end): no plan */
    std::vector<u8> small(kCellSize - 1, 0);
    put64(small, 0, kCellMarker);
    EXPECT_TRUE(plan_rewrites(small.data(), small.size(), 1, 1, 2).empty());

    /* null buffer / under-cell-size: no plan, no crash */
    EXPECT_TRUE(plan_rewrites(nullptr, 4096, 1, 1, 2).empty());
    std::vector<u8> tiny(16, 0);
    EXPECT_TRUE(plan_rewrites(tiny.data(), tiny.size(), 1, 1, 2).empty());

    /* the marker at a NON-multiple-of-8 position is never landed on
     * (the og's 8-byte walk) */
    auto hive = hive_with_cell(512 + 4, 0x55667788, 1000);
    EXPECT_TRUE(plan_rewrites(hive.data(), hive.size(), 0x55667788, 1000, 2000).empty());

    /* an empty plan executes/undoes OK without touching the writer */
    struct NeverWrite {
        static u32 thunk(u64, const u8*, void*) { return ERR_WRITE_FAILED; }
    };
    std::vector<BcdRewrite> empty;
    EXPECT_EQ(OK, execute_plan(empty, &NeverWrite::thunk, nullptr, nullptr, nullptr));
    EXPECT_EQ(OK, undo_plan(empty, &NeverWrite::thunk, nullptr));

    /* a writer error stops the plan and propagates */
    auto hive2 = hive_with_cell(512, 0x99AABBCC, 300);
    auto plan2 = plan_rewrites(hive2.data(), hive2.size(), 0x99AABBCC, 300, 301);
    ASSERT_EQ(plan2.size(), 1u);
    EXPECT_EQ(ERR_WRITE_FAILED,
              execute_plan(plan2, &NeverWrite::thunk, nullptr, allow, nullptr));
}
