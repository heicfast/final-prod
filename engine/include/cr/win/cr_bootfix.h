/*
 * cr_bootfix.h - the Windows-side boot-chain repair plumbing (M-040
 * stage a's win half + stage b's resolver).
 *
 * Corpus lineage (Fixup.mo / EFIBoot.mo, re-verified first-hand):
 *   - the BCD hive force-unload before patching (Fixup :243-255
 *     SeRestore/SeBackup enable; :533/:542 ZwUnloadKey on
 *     \Registry\Machine\BCD00000000 - the loaded hive would shadow the
 *     on-disk file the repair rewrites);
 *   - the post-patch flush chain (Fixup :263-306:
 *     FSCTL_ALLOW_EXTENDED_DASD_IO 0x90083 + FlushFileBuffers on the
 *     system partition);
 *   - the firmware-variable resolver (EFIBoot :3248-3264
 *     SeSystemEnvironmentPrivilege via LookupPrivilegeValueW +
 *     AdjustTokenPrivileges; :3281-3290 the dynamic
 *     GetProcAddress("SetFirmwareEnvironmentVariableW") XP-compat path;
 *     :3888-3894 the readiness probe - the zero-GUID dummy read must
 *     fail with something OTHER than ERROR_INVALID_FUNCTION).
 *
 * The pure logic lives in cr/bcdfix (stage a) and cr/efifix (stage b);
 * this layer owns handles, privileges, and the real firmware calls.
 * The consent model is the rawfixer/bcdfix one: DEFAULT-DENY without a
 * callback.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_WIN_BOOTFIX_H_
#define CR_WIN_BOOTFIX_H_

#include "../cr_types.h"
#include "../bcdfix/cr_bcdfix.h"
#include "../efifix/cr_efifix.h"
#include <cstddef>

namespace cr {
namespace win {
namespace bootfix {

/* Repair one BCD hive file (\Boot\BCD on a legacy boot partition,
 * \EFI\MICROSOFT\BOOT\BCD on an ESP) after a partition move: the
 * best-effort hive unload, the whole-file read, bcdfix::plan_rewrites,
 * the consent gate (nullptr = default-deny, ERR_REPAIR_REFUSED), the
 * OVERLAPPED-positioned 8-byte writes, and the flush tail. *applied
 * receives how many rewrites landed (0 = the hive does not reference
 * the partition - OK, the og's silent no-op). The hive-unload failure
 * degrades to proceed (logged through the cr::log seam): a WinPE or
 * dead-system repair has the target's hive unloaded anyway. */
u32 repair_bcd_file(const wchar_t* bcd_path, u32 mbr_signature, u64 old_start_lba,
                    u64 new_start_lba, bcdfix::ConsentFn consent, void* consent_user,
                    size_t* applied = nullptr);

/* The efifix firmware-variable resolver: the real
 * Get/SetFirmwareEnvironmentVariableW (EFI global GUID
 * {8BE4DF61-93CA-11d2-AA0D-00E098032B8C} is applied by the real API's
 * lpGuid parameter), SeSystemEnvironmentPrivilege enabled at
 * construction, and the readiness probe as the `available` slot.
 * Returns an incomplete vtable (complete() == false) only when the
 * privilege could not be enabled - update_boot_entries then refuses
 * with ERR_BAD_PARAMETER (never a silent no-op on a live EFI host). */
efifix::FirmwareVars make_firmware_vars();

/* The combined post-apply boot-chain repair (the og's Bcd apply chain
 * shape: Fixup's BCD rewrite + EFIBoot's NVRAM patch under one consent
 * decision). Both legs are individually consent-gated by the same
 * callback; either leg's failure is reported but does not block the
 * other (the og's chain continues past a Bcd failure). */
struct BootRepairResult {
    u32 bcd_rc = 0;             /* per-file: OK or the first error */
    size_t bcd_files_patched = 0;
    size_t bcd_cells_patched = 0;
    u32 nvram_rc = 0;
    efifix::NvramStats nvram;
};
u32 run_boot_chain_repair(const wchar_t* const* bcd_paths, size_t bcd_path_count,
                          u32 mbr_signature, u64 old_start_lba, u64 new_start_lba,
                          const efifix::PartitionRef& nvram_old,
                          const efifix::PartitionRef& nvram_new, bcdfix::ConsentFn consent,
                          void* consent_user, BootRepairResult* out = nullptr);

} // namespace bootfix
} // namespace win
} // namespace cr

#endif // CR_WIN_BOOTFIX_H_
