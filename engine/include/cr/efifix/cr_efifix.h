/*
 * cr_efifix.h - the UEFI NVRAM Boot#### load-option updater (M-040
 * stage b).
 *
 * Corpus lineage (EFIBoot.mo, a link-time library imported by the
 * recovery hosts - re-workspace/reversed/ghidra-mo/EFIBoot_decompiled.c,
 * every gate re-verified first-hand): after a partition move the og
 * patches the partition references inside the UEFI boot variables -
 * the Boot#### load options whose device path names the moved
 * partition get their HARDDRIVE node rewritten (partition number,
 * start LBA, size, signature), through Get/SetFirmwareEnvironmentVariableW
 * with the EFI global variable GUID {8BE4DF61-93CA-11d2-AA0D-00E098032B8C}
 * and SeSystemEnvironmentPrivilege enabled (LookupPrivilegeValueW +
 * AdjustTokenPrivileges, :3248-3352; the XP-era path resolved
 * GetProcAddress("...FirmwareEnvironmentVariableW") at :3281-3290).
 *
 * The matcher decode (FUN_1800046f0 :3366-3434): a Boot#### entry
 * matches when its EFI_LOAD_OPTION's device path (header u32@0 +
 * FilePathListLength u16@4 + the NUL-terminated UTF-16 description
 * starting @6, then the device path nodes) contains a MEDIA_HARDDRIVE_DP
 * node (type 0x04, subtype 0x01; node layout: partition number u32@+4,
 * start LBA u64@+8, size u64@+0x10, 16-byte signature @+0x18, format
 * u8@+0x28, signature type u8@+0x29) carrying the target partition:
 * MBR mode (format==1, sigtype==1) matches the u32 disk signature AND
 * the start LBA AND the size; GPT mode (format==2, sigtype==2) matches
 * the partition GUID only.
 *
 * The updater decode (FUN_180004b40 :3564-3715): walk BootOrder (the
 * u16 Boot#### id array); for each id whose entry matches the OLD
 * partition reference, re-read the variable, patch EVERY HARDDRIVE
 * node in its path to the NEW reference (partition number@+4, start
 * LBA@+8, size@+0x10, and the signature@+0x18 - the MBR form writes
 * the u32 signature only, the GPT form writes all 16 GUID bytes), and
 * write the variable back. BootOrder itself is never rewritten (only
 * the create/delete paths touch it - out of stage-b scope per the
 * finding's plan). Any firmware-op failure stops the sweep (the og's
 * three break sites) - each entry's write is atomic, partial
 * application is the og's own semantics.
 *
 * Safety posture (deliberate hardening over the og, registered with
 * the finding): the og walks the description and the node chain with
 * NO bounds checks - a hostile firmware variable (unterminated
 * description, zero/oversized node length) overreads the 1 MiB
 * scratch buffer. The core clamps every walk to the buffer end and
 * treats a malformed option as non-matching, never as a crash. The
 * consent gate is the bcdfix/rawfixer model verbatim: no callback =
 * DEFAULT-DENY, ERR_REPAIR_REFUSED (P62).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_EFIXIF_H_
#define CR_EFIXIF_H_

#include "../cr_types.h"
#include <cstddef>
#include <vector>

namespace cr {
namespace efifix {

/* The og's 1 MiB scratch per firmware-variable read (every
 * GetFirmwareEnvironmentVariableW call passes 0x100000). */
constexpr size_t kMaxLoadOptionBytes = 0x100000;

/* The injected firmware-variable surface. The win layer provides the
 * real Get/SetFirmwareEnvironmentVariableW (+ the privilege enable +
 * the readiness probe); tests provide an in-memory fake. */
struct FirmwareVars {
    /* Readiness (EFIBoot :3888-3894 decode): the zero-GUID dummy read
     * must fail with something OTHER than ERROR_INVALID_FUNCTION (=1) -
     * i.e. the platform HAS firmware variables (EFI, not legacy BIOS).
     * false = the updater no-ops (the og's silent-return gate). */
    bool (*available)(void* user) = nullptr;
    /* Read variable `name` into out (cap bytes); returns the size read,
     * 0 on absence/failure (the GetFirmwareEnvironmentVariableW
     * contract). */
    u32 (*get)(void* user, const char16_t* name, u8* out, u32 cap) = nullptr;
    /* Write `name` <- data[len] (SetFirmwareEnvironmentVariableW);
     * false = failure. */
    bool (*set)(void* user, const char16_t* name, const u8* data, u32 len) = nullptr;
    void* user = nullptr;

    bool complete() const { return available != nullptr && get != nullptr && set != nullptr; }
};

/* The partition reference (the og matcher's ctx fields / the updater's
 * new-info struct, decoded): MBR mode matches signature AND start AND
 * size; GPT mode matches the partition GUID only. */
struct PartitionRef {
    u8 signature[16] = {0}; /* MBR: the u32 disk signature lives in the
                             * first 4 bytes (the node's remaining 12
                             * signature bytes are zero on MBR nodes);
                             * GPT: the full 16-byte partition GUID */
    u32 partition_number = 0;
    u64 start_lba = 0;
    u64 size_lbas = 0;
    bool gpt = false;
};

/* The consent callback (the bcdfix/rawfixer model): return true to
 * allow the write; false = refused. */
using ConsentFn = bool (*)(void* user);

/* Match a Boot#### load option against the partition reference
 * (FUN_1800046f0). The device-path walk is bounds-clamped: a malformed
 * option (no description terminator, invalid node lengths) is
 * non-matching, never an overread. */
bool boot_entry_matches(const PartitionRef& ref, const u8* load_option, u32 len);

/* Outcome telemetry. */
struct NvramStats {
    u32 entries_scanned = 0; /* BootOrder ids visited */
    u32 entries_matched = 0; /* ids whose load option matched old_ref */
    u32 entries_patched = 0; /* variables actually rewritten */
};

/* The updater (FUN_180004b40): walk BootOrder; for each matching
 * Boot####, patch every HARDDRIVE node to new_ref and write the
 * variable back. Consent-gated (nullptr = default-deny,
 * ERR_REPAIR_REFUSED, nothing written). Returns OK on completion (0
 * patched = no entry referenced the partition - the og's silent
 * no-op), ERR_BAD_PARAMETER on an incomplete resolver; a firmware
 * failure stops the sweep (the og's break semantics - entries already
 * patched stay patched) and reports through stats->entries_patched. */
u32 update_boot_entries(const FirmwareVars& fw, const PartitionRef& old_ref,
                        const PartitionRef& new_ref, ConsentFn consent, void* consent_user,
                        NvramStats* stats = nullptr);

} // namespace efifix
} // namespace cr

#endif // CR_EFIXIF_H_
