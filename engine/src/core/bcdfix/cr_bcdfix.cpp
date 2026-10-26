/* cr_bcdfix.cpp - the BCD partition-offset rewriter (M-040 stage a).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/bcdfix/cr_bcdfix.h"
#include "cr/cr_error.h"
#include <cstring>

namespace cr {
namespace bcdfix {

namespace {

u64 le64_at(const u8* p) {
    u64 v = 0;
    for (int i = 7; i >= 0; --i)
        v = (v << 8) | u64(p[i]);
    return v;
}

u32 le32_at(const u8* p) {
    return u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24);
}

void put64(u8 (&b)[8], u64 v) {
    for (int i = 0; i < 8; ++i)
        b[i] = u8((v >> (8 * i)) & 0xFF);
}

} // namespace

std::vector<BcdRewrite> plan_rewrites(const u8* hive, size_t size, u32 mbr_signature,
                                      u64 old_start_lba, u64 new_start_lba) {
    std::vector<BcdRewrite> plan;
    if (hive == nullptr || size < kCellSize)
        return plan;
    /* the og walks the hive 8 bytes at a time reading cell markers
     * (FUN_180001980 :545-596). The +0x24 offset field and the +0x3C
     * signature field are both inside the 96-byte cell, so every
     * position with marker+kCellSize <= size is examined the same way
     * the og's 8-byte walk lands on it. */
    for (size_t p = 0; p + kCellSize <= size; p += kMarkerSize) {
        if (le64_at(hive + p) != kCellMarker)
            continue;
        const u8* cell = hive + p;
        const u64 part_bytes = le64_at(cell + kOffsetFieldInCell);
        const u32 sig = le32_at(cell + kSignatureFieldInCell);
        /* the og's match: signature AND (offset >> 9) == old LBA
         * (:571-584: lVar6 = (offset + sign-fixup) >> 9 compared to the
         * partition start; the sign-fixup is the >> of a negative-safe
         * division by 512, value-equivalent to offset >> 9 for the
         * positive offsets real hives carry) */
        if (sig != mbr_signature || (part_bytes >> 9) != old_start_lba)
            continue;
        BcdRewrite r;
        r.field_offset = u64(p) + kOffsetFieldInCell;
        std::memcpy(r.old_bytes, cell + kOffsetFieldInCell, 8);
        put64(r.new_bytes, new_start_lba << 9);
        plan.push_back(r);
    }
    return plan;
}

u32 execute_plan(const std::vector<BcdRewrite>& plan, WriteFn write, void* user,
                 ConsentFn consent, void* consent_user, size_t* applied) {
    if (applied != nullptr)
        *applied = 0;
    if (plan.empty())
        return OK; /* nothing to do - the BCD does not reference the partition */
    if (write == nullptr)
        return ERR_BAD_PARAMETER;
    /* P62/the rawfixer S-008 model: NO consent callback = default-DENY.
     * A write-side family that can brick a boot chain never runs on a
     * library default. */
    if (consent == nullptr || !consent(consent_user))
        return ERR_REPAIR_REFUSED; // the rawfixer consent-refusal family code
    for (const BcdRewrite& r : plan) {
        const u32 rc = write(r.field_offset, r.new_bytes, user);
        if (rc != OK)
            return rc;
        if (applied != nullptr)
            ++*applied;
    }
    return OK;
}

u32 undo_plan(const std::vector<BcdRewrite>& plan, WriteFn write, void* user,
              size_t* applied) {
    if (applied != nullptr)
        *applied = 0;
    if (plan.empty())
        return OK;
    if (write == nullptr)
        return ERR_BAD_PARAMETER;
    /* the undo restores the ORIGINAL bytes - Bcd_RestoreBcdFile's swap
     * (Fixup :4562-4625). Undo is itself a write to the same hive: the
     * host invokes it from its own consented undo flow (the rawfixer
     * undo contract - the recorded undo log IS the authorization). */
    for (size_t i = plan.size(); i-- > 0;) {
        const BcdRewrite& r = plan[i];
        const u32 rc = write(r.field_offset, r.old_bytes, user);
        if (rc != OK)
            return rc;
        if (applied != nullptr)
            ++*applied;
    }
    return OK;
}

} // namespace bcdfix
} // namespace cr
