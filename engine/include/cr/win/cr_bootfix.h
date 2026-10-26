/*
 * cr_bootfix.h - the post-recovery boot-chain repair family (M-040,
 * the win half: the runtime plumbing under the pure cr/bcdfix +
 * cr/nvramfix modules).
 *
 * Corpus lineage (Fixup.mo + EFIBoot.mo, decompile-verified):
 *   - The BCD offset rewriter (Fixup FUN_180002520/FUN_180001980):
 *     read \Boot\BCD / \EFI\MICROSOFT\BOOT\BCD, plan the device-element
 *     cell rewrites, write (new_lba << 9), flush.
 *   - The BCD hive force-unload (Fixup :527-546): when the BCD file
 *     cannot be opened for write (the loader has it mounted),
 *     SeBackupPrivilege + SeRestorePrivilege are enabled and the
 *     mounted hive \Registry\Machine\BCD00000000 is unloaded with
 *     ZwUnloadKey before retrying the open.
 *   - The UEFI NVRAM Boot#### updater (EFIBoot FUN_180004840): walk
 *     BootOrder through GetFirmwareEnvironmentVariableW, patch matched
 *     load options, write back with SetFirmwareEnvironmentVariableW —
 *     both resolved DYNAMICALLY (EFIBoot :3281-3290: the exports do not
 *     exist on legacy-BIOS systems) with SeSystemEnvironmentPrivilege
 *     enabled (:3248-3270). Dynamic resolution also keeps this module's
 *     PE import surface Win7-clean (no new import; the allowlist gate
 *     is untouched).
 *
 * CONSENT: every write path takes the caller's explicit gate — the
 * bcdfix ConsentFn (default-DENY, the rawfixer S-008 model); the NVRAM
 * pass is a separate explicit call (hosts opt in per operation).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_WIN_BOOTFIX_H_
#define CR_WIN_BOOTFIX_H_

#include "../cr_types.h"
#include "../bcdfix/cr_bcdfix.h"
#include "../nvramfix/cr_nvramfix.h"

namespace cr {
namespace win {
namespace bootfix {

/* The firmware-variable store: dynamically resolved
 * Get/SetFirmwareEnvironmentVariableW + SeSystemEnvironmentPrivilege.
 * is_usable() is false on legacy-BIOS systems (ERROR_INVALID_FUNCTION)
 * — every operation through an unusable store is a clean refusal, never
 * a crash. */
class FwVarStore final : public nvramfix::IFirmwareVarStore {
public:
    FwVarStore();

    bool is_usable() const { return get_ != nullptr && set_ != nullptr; }

    /* IFirmwareVarStore contract (cr_nvramfix.h):
     *   OK / ERR_OPEN_FAILED (variable absent) / other (API unusable). */
    u32 get(const wchar_t* name, void* buf, u32 buf_len, u32* out_len) override;
    u32 set(const wchar_t* name, const void* buf, u32 len) override;

private:
    /* the SDK signatures, expressed in portable types (DWORD == u32 on
     * every supported target); assigned from GetProcAddress at
     * construction. */
    u32 (*get_)(const wchar_t*, const wchar_t*, void*, u32, u32*) = nullptr;
    bool (*set_)(const wchar_t*, const wchar_t*, const void*, u32) = nullptr;
    bool privileged_ = false;
};

/* The state-4 NVRAM renumber pass (M-040 stage b): one explicit call —
 * enable privilege, open the store, run cr::nvramfix::
 * update_boot_entries. ERR_UNSUPPORTED on BIOS systems (stats zeroed);
 * the store's own errors propagate. */
u32 nvram_renumber(const nvramfix::Target& t, nvramfix::UpdateStats* stats);

/* Rewrite the partition offsets in ONE BCD hive file (M-040 stage a's
 * win half): read the file, plan (bcdfix::plan_rewrites), execute
 * through the file writer under the consent callback, FlushFileBuffers
 * (Fixup :263-306 tail). Consent == nullptr is DEFAULT-DENY. The file
 * is expected at the caller-supplied path (\Boot\BCD on the legacy
 * boot partition, \EFI\MICROSOFT\BOOT\BCD on the ESP — the host
 * resolves the partition letters). */
u32 bcd_rewrite_file(const wchar_t* bcd_path, u32 mbr_signature, u64 old_start_lba,
                     u64 new_start_lba, bcdfix::ConsentFn consent, void* consent_user,
                     size_t* applied);

/* Force-unload the loader-mounted BCD hive (Fixup :527-546):
 * SeBackupPrivilege + SeRestorePrivilege, then ZwUnloadKey over
 * \Registry\Machine\BCD00000000 (ntdll resolved dynamically — no new
 * PE import). Returns false when the unload is not possible (no
 * privilege / API absent); the caller retries the file open either way
 * (the og retries the open up to 3 times regardless). */
bool unload_bcd_hive();

} // namespace bootfix
} // namespace win
} // namespace cr

#endif // CR_WIN_BOOTFIX_H_
