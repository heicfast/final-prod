/*
 * cr_bcdfix.h - the BCD partition-offset rewriter (M-040 stage a).
 *
 * Corpus lineage (Fixup.mo, a link-time library imported by six hosts -
 * re-workspace/reversed/ghidra-mo/Fixup_decompiled.c, re-verified
 * first-hand): after a partition move/resize the og binary-patches the
 * partition offsets inside the BCD hives (\Boot\BCD on the legacy boot
 * partition, \EFI\MICROSOFT\BOOT\BCD on the ESP).
 *
 * The cell decode (FUN_180001980 :499-599): the hive is walked 8 bytes
 * at a time; an ALLOCATED-cell marker of 0xffffffa0 (a 96-byte cell,
 * two's-complement -0x60) announces a candidate device element. Within
 * the cell (marker at cell start P): the partition byte-offset field is
 * the u64 at P+0x24 (stored as BYTES - the consumer derives LBA as
 * value >> 9); the MBR disk signature is the u32 at P+0x3C. A cell
 * belongs to the moved partition when signature == the disk's MBR
 * signature AND (offset >> 9) == the partition's old start LBA.
 *
 * The rewrite (FUN_180002520 :957-992): seek back 0x1c from
 * after-the-signature (landing on P+0x24), WriteFile 8 bytes of
 * (new_lba << 9), then advance +0x14/+0x18 past the cell for the next
 * candidate.
 *
 * Plan/execute/undo shape (cr_rawfixer's): scan builds the PLAN (every
 * matching cell's absolute file offset + the original 8 bytes + the
 * replacement); execute applies the plan through an injected writer
 * under the host's CONSENT callback (default-deny - P62, the rawfixer
 * S-008 consent model); undo restores the original bytes. Nothing is
 * written outside an explicitly-executed plan.
 *
 * Stage scope (the finding's own fix plan): this is stage (a) - the
 * BCD offset rewriter only, "the narrowest recovery-relevant piece".
 * Stage (b) (UEFI NVRAM Boot####/BootOrder patching) is the recorded
 * next step (tracked-open with the decoded state-machine citations);
 * boot.ini is registered skippable (Win7 boots BCD - the plan's own
 * note); the post-op ChkDsk.exe delegation is registered UI-layer
 * responsibility (an external binary, out of core scope).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_BCDFIX_H_
#define CR_BCDFIX_H_

#include "../cr_types.h"
#include <cstddef>
#include <vector>

namespace cr {
namespace bcdfix {

/* The hive-cell shape (Fixup :499-599 decode, first-hand this session). */
constexpr u64 kCellMarker = 0xffffffa0u; /* allocated 96-byte cell */
constexpr size_t kMarkerSize = 8;        /* the walked 8-byte cell header */
constexpr size_t kOffsetFieldInCell = 0x24; /* u64 partition BYTE offset */
constexpr size_t kSignatureFieldInCell = 0x3C; /* u32 MBR disk signature */
constexpr size_t kCellSize = 0x58;       /* 8 header + 0x50 body = 96 */

/* One planned rewrite: the absolute byte offset of the offset-field
 * inside the hive, its original 8 bytes, and the replacement. The undo
 * record IS the original bytes - byte-exact restore. */
struct BcdRewrite {
    u64 field_offset = 0; /* absolute file offset of the u64 offset field */
    u8 old_bytes[8] = {0};
    u8 new_bytes[8] = {0};
};

/* The consent callback: return true to allow the write (the host's
 * explicit gate - P62, the rawfixer S-008 model); false = refused. */
using ConsentFn = bool (*)(void* user);

/* Scan a whole hive buffer for device-element cells of the target
 * partition (MBR signature + old start LBA) and build the rewrite plan
 * (new_lba << 9 at every matching cell's offset field, Fixup :957-992).
 * Reads nothing else; the buffer is the whole \Boot\BCD /
 * \EFI\MICROSOFT\BOOT\BCD file. Returns the plan (empty = no matching
 * cell: the BCD does not reference this partition). */
std::vector<BcdRewrite> plan_rewrites(const u8* hive, size_t size, u32 mbr_signature,
                                      u64 old_start_lba, u64 new_start_lba);

/* The injected writer: write 8 bytes at an absolute offset into the
 * target hive (the win layer owns the handle + the post-patch
 * FSCTL_ALLOW_EXTENDED_DASD_IO 0x90083 + FlushFileBuffers tail,
 * Fixup :263-306). Returns OK or an error code. */
using WriteFn = u32 (*)(u64 field_offset, const u8 bytes[8], void* user);

/* Execute a plan through the writer under the consent callback.
 * consent == nullptr means DEFAULT-DENY (nothing is written - the
 * conservative library default). Stops at the first write error and
 * returns it; *applied receives how many rewrites landed. Refused by
 * consent returns ERR_REPAIR_REFUSED (the rawfixer consent family code)
 * with applied == 0. */
u32 execute_plan(const std::vector<BcdRewrite>& plan, WriteFn write, void* user,
                 ConsentFn consent, void* consent_user, size_t* applied = nullptr);

/* Undo: restore every original byte the plan recorded (the
 * Bcd_RestoreBcdFile analog, Fixup :4562-4625 - swap old/new back). */
u32 undo_plan(const std::vector<BcdRewrite>& plan, WriteFn write, void* user,
              size_t* applied = nullptr);

} // namespace bcdfix
} // namespace cr

#endif // CR_BCDFIX_H_
