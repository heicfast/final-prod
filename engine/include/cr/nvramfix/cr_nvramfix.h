/*
 * cr_nvramfix.h - the UEFI NVRAM Boot####/BootOrder updater (M-040 stage b).
 *
 * Corpus lineage (EFIBoot.mo, re-workspace/reversed/ghidra-mo/
 * EFIBoot_decompiled.c, decoded first-hand this session):
 *
 *  - The firmware seam (:3281-3290): Get/SetFirmwareEnvironmentVariableW
 *    are resolved DYNAMICALLY from kernel32 (GetProcAddress) - an XP-era
 *    box without the exports still loads; the EFI global variable GUID
 *    {8BE4DF61-93CA-11d2-AA0D-00E098032B8C} namespaces every query
 *    (:3385+). This module keeps the seam INJECTED (function pointers)
 *    so the pure logic is testable and the win layer owns the dynamic
 *    resolution.
 *
 *  - The candidate set (:3493-3520 / :3630-3654 / :3783+): the
 *    BootOrder variable (a u16 array) is read, and every entry is
 *    formatted "Boot%04X" for inspection. Nothing outside BootOrder is
 *    ever touched (no create/delete - the finding's stage-b scope).
 *
 *  - The matcher (FUN_1800046f0 :3366-3445): the Boot#### variable's
 *    EFI_LOAD_OPTION is walked - the u16 Description at +6 is skipped
 *    (walk to its NUL, step one), then the device path nodes are walked
 *    by their u16 Length. A MEDIA_HARDDRIVE node (Type 4, SubType 1)
 *    matches the target partition when:
 *      MBR mode (SignatureType==1, MBRType==1): node.Signature[0..4] ==
 *        the disk's MBR signature AND node.PartitionStart ==
 *        old_start AND node.PartitionSize == old_size;
 *      GPT mode (SignatureType==2, MBRType==2): node.Signature[0..16] ==
 *        the partition GUID AND node.PartitionStart == old_start.
 *    (Node field offsets: PartitionNumber u32@4, PartitionStart u64@8,
 *    PartitionSize u64@16, Signature[16]@24, MBRType u8@40,
 *    SignatureType u8@41 - the UEFI MEDIA_HARDDRIVE_DEVICE_PATH; the
 *    decompile's psVar6[0xc]/[+4]/[+8]/[+0x10]/[0x29] map onto them.)
 *
 *  - The rewrite (FUN_180004b40's patch loop :3660-3700): in every
 *    MATCHED entry, each HARDDRIVE node's PartitionNumber,
 *    PartitionStart, PartitionSize and Signature are overwritten with
 *    the NEW partition's values (MBR: the new disk signature; GPT: the
 *    new GUID), then the whole variable is written back through
 *    SetFirmwareEnvironmentVariableW. The sibling loop at :3520-3572
 *    rewrites ONLY PartitionNumber (a renumber-only pass the og also
 *    has); this module implements the full-position rewrite (the
 *    recovery-relevant one - the partition moved).
 *
 *  - The safety model: plan/execute/undo over the injected seam, with
 *    the P62 default-deny consent callback (the rawfixer S-008
 *    discipline). The undo restores the ORIGINAL variable bytes
 *    byte-exactly.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_NVRAMFIX_H_
#define CR_NVRAMFIX_H_

#include "../cr_types.h"
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

namespace cr {
namespace nvramfix {

/* ---- UEFI device path constants (spec 2.x + the corpus decode) ---- */
constexpr u8 kTypeEndEntire = 0x7f;
constexpr u8 kSubTypeEndEntire = 0xff;
constexpr u8 kTypeMedia = 0x04;
constexpr u8 kSubTypeHarddrive = 0x01;
constexpr size_t kNodeHeaderSize = 4; /* u8 type, u8 subtype, u16 len */
/* MEDIA_HARDDRIVE_DEVICE_PATH fields */
constexpr size_t kHdPartitionNumber = 4; /* u32 */
constexpr size_t kHdPartitionStart = 8;  /* u64 */
constexpr size_t kHdPartitionSize = 16;  /* u64 */
constexpr size_t kHdSignature = 24;      /* u8[16] */
constexpr size_t kHdMbrType = 40;        /* u8: 1=MBR, 2=GPT */
constexpr size_t kHdSignatureType = 41;  /* u8: 1=MBR sig, 2=GUID, 0=none */
constexpr size_t kHdNodeSize = 42;       /* the spec's fixed size */

/* EFI_LOAD_OPTION: u32 Attributes@0, u16 FilePathListLength@4, the
 * UTF-16 Description starts at +6 (NUL-terminated), then the device
 * path byte stream (the corpus walk: buf+6, u16s to NUL, +1). */
constexpr size_t kLoadOptDescription = 6;

constexpr size_t kMaxVariableBytes = 1 << 20; /* the og's 0x100000 cap */
constexpr size_t kMaxBootEntries = 0xffff;    /* the og's ushort entry-count
                                            * truncation (FUN_180004b40 :3637) */

/* The moved partition: the OLD identity (the match keys) + the NEW
 * position/signature (the rewrite values). Exactly one of the two
 * signature modes is armed per target (mbr vs gpt), mirroring the og's
 * ctx+0x30 mode switch. */
struct TargetPartition {
    /* OLD identity - the match keys */
    u32 old_mbr_signature = 0; /* MBR mode */
    u8 old_gpt_guid[16] = {0}; /* GPT mode */
    bool gpt_mode = false;     /* false = MBR matching, true = GPT */
    u64 old_start = 0;
    u64 old_size = 0;
    /* NEW values - the rewrite payload */
    u32 new_partition_number = 0;
    u64 new_start = 0;
    u64 new_size = 0;
    u32 new_mbr_signature = 0; /* MBR-mode rewrite */
    u8 new_gpt_guid[16] = {0}; /* GPT-mode rewrite */
};

/* The injected firmware-variable seam (the win layer resolves
 * Get/SetFirmwareEnvironmentVariableW dynamically, corpus :3281-3290).
 * Get returns the variable's byte length (0 = absent/too small); Set
 * returns true on success. */
using GetVarFn = u32 (*)(const char16_t* name, void* buf, u32 cap, void* user);
using SetVarFn = bool (*)(const char16_t* name, const void* data, u32 len, void* user);

/* The consent callback (P62, the rawfixer S-008 model): return true to
 * allow the writes; nullptr or false = refused, nothing is written. */
using ConsentFn = bool (*)(void* user);

/* One planned rewrite: the variable's name, its ORIGINAL bytes (the
 * undo record) and the PATCHED bytes. */
struct NvramRewrite {
    std::u16string name;
    std::vector<u8> old_value;
    std::vector<u8> new_value;
};

/* Build the rewrite plan: walk BootOrder, match every Boot#### entry's
 * HARDDRIVE node against the target's OLD identity, and produce the
 * patched entry payloads. Reads nothing but the variables it inspects;
 * writes nothing (planning only). Returns the plan (empty = no
 * reference to this partition in any Boot#### entry).
 *
 * Errors: ERR_BAD_PARAMETER (null seam), ERR_READ_FAILED (BootOrder
 * unreadable), ERR_BAD_MAGIC (a load option too malformed to walk). */
std::vector<NvramRewrite> plan_boot_entries(GetVarFn get, void* get_user,
<<<<<<< HEAD
                                             const TargetPartition& target, u32* error = nullptr,
                                             bool* stopped_early = nullptr);
=======
                                            const TargetPartition& target, u32* error = nullptr);
>>>>>>> origin/main

/* Execute a plan through the injected setter under the consent
 * callback. consent == nullptr = DEFAULT-DENY. Stops at the first
 * failed Set (returns ERR_WRITE_FAILED) with *applied counting the
 * landed rewrites. Refused by consent returns ERR_REPAIR_REFUSED with
 * applied == 0. */
u32 execute_plan(const std::vector<NvramRewrite>& plan, SetVarFn set, void* set_user,
                 ConsentFn consent, void* consent_user, size_t* applied = nullptr);

/* Undo: restore every variable's ORIGINAL bytes (byte-exact). */
u32 undo_plan(const std::vector<NvramRewrite>& plan, SetVarFn set, void* set_user,
              size_t* applied = nullptr);

} // namespace nvramfix
} // namespace cr

#endif // CR_NVRAMFIX_H_
