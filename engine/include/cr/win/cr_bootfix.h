/*
 * cr_bootfix.h - the Windows side of the post-recovery boot-chain repair
 * (M-040 stages a+b): the BCD hive-file rewrites and the UEFI NVRAM
 * variable seam.
 *
 * Stage a (the pure cr/bcdfix module) plans/executes/undoes the
 * partition-offset rewrites; this layer owns the FILE side: open the
 * hive (\Boot\BCD or \EFI\MICROSOFT\BOOT\BCD), read it, apply the plan
 * positionally, flush (the og's post-patch tail, Fixup :263-306 - the
 * FSCTL_ALLOW_EXTENDED_DASD_IO 0x90083 half applies to the og's VOLUME
 * handle; a hive FILE gets FlushFileBuffers, the same durability
 * intent), and - when the registry loader holds the hive - the
 * \Registry\Machine\BCD00000000 force-unload (ZwUnloadKey with
 * SeRestore/SeBackup enabled, Fixup :533/:542/:243-255).
 *
 * Stage b (the pure cr/nvramfix module) plans/executes/undoes the
 * Boot#### load-option rewrites; this layer owns the firmware-variable
 * seam: Get/SetFirmwareEnvironmentVariableW resolved DYNAMICALLY from
 * kernel32 (EFIBoot :3281-3290 - the og's own pattern, keeping both
 * off our static import table), with SeSystemEnvironmentPrivilege
 * enabled through ntdll!RtlAdjustPrivilege (the og's XP path,
 * :3248-3270).
 *
 * SAFETY (P62, the rawfixer S-008 discipline): every execute is gated
 * by the explicit consent callback; nullptr/false = refused with
 * ERR_REPAIR_REFUSED and ZERO writes. Undo restores the original bytes
 * byte-exactly.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_WIN_BOOTFIX_H_
#define CR_WIN_BOOTFIX_H_

#include "../bcdfix/cr_bcdfix.h"
#include "../nvramfix/cr_nvramfix.h"
#include "../cr_types.h"
#include <cstddef>
#include <vector>

namespace cr {
namespace win {
namespace bootfix {

/* ---- stage a: the BCD hive file ------------------------------------- */

/* Read the hive file and build the rewrite plan (planning only - no
 * write, no consent needed). Returns OK / ERR_READ_FAILED (open or
 * read failed; GetLastError is in *last_error when provided). */
u32 bcd_file_plan(const wchar_t* path, u32 mbr_signature, u64 old_start_lba, u64 new_start_lba,
                  std::vector<bcdfix::BcdRewrite>* out, unsigned long* last_error = nullptr);

/* Execute the plan against the hive file under the consent callback
 * (default-deny). When the file is held by the registry loader, the
 * \Registry\Machine\BCD00000000 force-unload runs once and the open
 * retries (the og's Fixup :533/:542 pattern). FlushFileBuffers after
 * the last write. Stops at the first write failure with *applied
 * counting the landed rewrites. */
u32 bcd_file_execute(const wchar_t* path, const std::vector<bcdfix::BcdRewrite>& plan,
                     bcdfix::ConsentFn consent, void* consent_user, size_t* applied = nullptr,
                     unsigned long* last_error = nullptr);

/* Undo: restore the plan's original bytes (byte-exact) + flush. */
u32 bcd_file_undo(const wchar_t* path, const std::vector<bcdfix::BcdRewrite>& plan,
                  size_t* applied = nullptr, unsigned long* last_error = nullptr);

/* The force-unload alone (exposed for the test seam on Windows CI):
 * ZwUnloadKey(\Registry\Machine\BCD00000000) with SeRestore/SeBackup
 * enabled. Returns true when the key was unloaded OR was not loaded
 * (STATUS_OBJECT_NAME_NOT_FOUND is success for our purpose). Dynamic
 * ntdll resolution only - no new static imports. */
bool force_unload_bcd_hive();

/* ---- stage b: the UEFI NVRAM variables -------------------------------- */

/* Are the firmware-variable entry points available on this system?
 * (GetFirmwareEnvironmentVariableW exists; false on legacy BIOS or a
 * pre-Vista kernel - the caller reports capability, not failure.) */
bool firmware_seam_available();

/* Enable SeSystemEnvironmentPrivilege for this process (the og's XP
 * path, ntdll!RtlAdjustPrivilege). Returns true when the privilege is
 * (already) held and enabled; the plan/execute calls check this and
 * refuse cleanly when false. */
bool enable_system_environment_privilege();

/* Plan over the live firmware variables (pure cr::nvramfix with the
 * resolved Get seam). Returns the plan; *error carries the cr error
 * code. */
std::vector<nvramfix::NvramRewrite> nvram_plan(const nvramfix::TargetPartition& target,
                                               u32* error = nullptr, bool* stopped_early = nullptr);

/* Execute the plan through the live Set seam under the consent
 * callback (default-deny). */
u32 nvram_execute(const std::vector<nvramfix::NvramRewrite>& plan, nvramfix::ConsentFn consent,
                  void* consent_user, size_t* applied = nullptr);

/* Undo: restore the original variable bytes. */
u32 nvram_undo(const std::vector<nvramfix::NvramRewrite>& plan, size_t* applied = nullptr);

} // namespace bootfix
} // namespace win
} // namespace cr

#endif // CR_WIN_BOOTFIX_H_
