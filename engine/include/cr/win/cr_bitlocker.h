/*
 * cr_bitlocker.h - BitLocker-aware handling (R8).
 *
 * Policy (fix F13): DETECT + LEGITIMATE WINDOWS UNLOCK ONLY. Detection via
 * FVE metadata ("-FVE-FS-" OEM at 0x03, corpus signature). Unlock paths:
 *   1. PowerShell Unlock-BitLocker (password/recovery key) - primary
 *   2. manage-bde -unlock - fallback (corpus legacy path)
 *   3. FVEAutoUnlock registry external keys (live hive AND offline hive -
 *      S-014: the og's DRWService reads
 *      HKLM\SYSTEM\CurrentControlSet\Control\FVEAutoUnlock\{volume-guid}\Data
 *      and bitlockerhelp mounts a dead system's own
 *      <root>\Windows\System32\config\SYSTEM hive for the same family)
 * NEVER any decryption-bypass code. Locked volumes report ERR_FVE_LOCKED.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_BITLOCKER_H_
#define CR_BITLOCKER_H_

#include "../cr_types.h"
#include "../reghive/cr_reghive.h"
#include "../recovery/cr_dict.h"
#include <string>

namespace cr {
namespace win {
namespace bitlocker {

/* Detection: FVE boot signature in the volume's first sector. */
bool is_fve_volume(wchar_t letter);

/* Status probe: locked / encrypted / clear. */
enum class Status : u8 { Clear = 0, Locked, UnlockedEncrypted, Unknown };
Status status(wchar_t letter);

/* Attempt legitimate unlock. key_type: 0 password, 1 recovery key (48-digit
 * string), 2 BEK file path. Returns OK or ERR_FVE_LOCKED on failure.
 * We never touch the crypto layer - Windows applies the keys. */
u32 unlock(wchar_t letter, int key_type, const std::wstring& key_or_path);

/* Auto-unlock (live + offline, S-014): read the per-volume external-key
 * blobs from HKLM\SYSTEM\CurrentControlSet\Control\FVEAutoUnlock (the og's
 * DRWService path, DRWService_decompiled.c:104); when the live registry has
 * no matching key, probe every other mounted volume's offline SYSTEM hive
 * (<X>:\Windows\System32\config\SYSTEM - the og's bitlockerhelp flow,
 * bitlockerhelp_decompiled.c:3128-3155) and try the harvested keys. The
 * target volume's own GUID (GetVolumeNameForVolumeMountPoint) is preferred
 * for record selection, others follow (og caps: 100 records / 256-byte
 * blobs / 50-char GUIDs). */
u32 try_auto_unlock(wchar_t letter);

/* Offline-only variant with an explicit hive file (dead-system scans where
 * the caller knows the OS root; the harvest walks the og's hardcoded
 * \ControlSet001\Control\FVEAutoUnlock family). Returns OK when a key from
 * the hive unlocked the volume, ERR_FVE_LOCKED otherwise. */
u32 try_auto_unlock_offline(wchar_t letter, const wchar_t* hive_path);

/* N-064 (diff-to-fix): the password-dictionary unlock aid - the og's Dict
 * flow (228-entry shipped dictionary, config-data-reference.md :20; tried
 * through the same PowerShell -Password / manage-bde -Password plumbing as
 * a user-typed password). This is the EXPLICIT CONSENT ACTION: nothing in
 * the scan/open path calls it - a locked volume still refuses with
 * ERR_FVE_LOCKED until the host invokes this (the CLI's --bitlocker-dict
 * flag / the cr_ui dictionary action). Candidates: the shipped dictionary
 * first (corpus order), then the user-provided strings. Every attempt
 * is logged through the cr::log seam (index only - password material is
 * never logged). Returns OK when a candidate unlocked the volume,
 * ERR_FVE_LOCKED when the whole set failed.
 *
 * CONVERGENCE (D-80, first-landing-wins): the parallel DFX34 lane's
 * cr::pwddict design was reconciled away in this session's N-064
 * convergence commit - its loader capability is absorbed here
 * (--bitlocker-dict [file]: the shipped table plus an og-format
 * dictionary file). Do NOT re-introduce a second dictionary module or
 * flag; extend THIS surface. */
u32 unlock_with_dictionary(wchar_t letter, const wchar_t* const* user_passwords,
                           size_t user_count);


/* Narrow-candidate overload (the CLI path; UTF-8/ASCII candidates from a
 * loaded dictionary file ride the same consent loop). */
u32 unlock_with_dictionary(wchar_t letter, const std::vector<std::string>& user_candidates);

#ifdef _WIN32
namespace detail {
/* ---- test seams (Windows lane only; see test_bitlocker_win.cpp) ---- */

/* The command builder for the password unlock forms (key_type 0 = -Password),
 * exposed for shape tests without spawning PowerShell. */
std::wstring build_unlock_command(wchar_t letter, int key_type,
                                  const std::wstring& key_or_path, bool legacy_manage_bde);

/* The dictionary loop with the attempt seam (the recorder surface for the
 * N-064 contract tests). Candidates are converted to wide and fed to
 * `attempt`; returns the cr::dict::AttemptResult. */
cr::dict::AttemptResult dictionary_unlock_seam(
    const std::vector<std::string>& user_candidates,
    u32 (*attempt)(wchar_t letter, const wchar_t* password, void* user), wchar_t letter,
    void* user);

/* The og's live registry path constant (ASCII view for tests). */
const char* fve_live_registry_path_a();

/* Enumerate per-volume-GUID "Data" blobs under an OPENED FVEAutoUnlock
 * root key (any hive root - tests use an HKCU fixture). Corpus semantics
 * (DRWService_decompiled.c:104-140): subkeys are volume GUIDs (uppercased
 * like the og's _strupr_s), each carries a REG_BINARY "Data" value; the
 * pipe record caps are 50-char GUID, 256-byte blob, 100 records
 * (artifacts/DRWService-exe--bitlocker-service.md :56-74). */
size_t enum_auto_unlock_keys(void* hkey_root, std::vector<reghive::AutoUnlockKey>& out);

/* Apply harvested keys: matching-GUID first, then the rest; each blob is
 * written to a temp .BEK and `attempt` is invoked (the real flow passes
 * nullptr -> the PowerShell unlock; tests pass a recorder). Returns OK on
 * the first success, ERR_FVE_LOCKED otherwise. */
u32 apply_auto_unlock_keys(wchar_t letter, const std::vector<reghive::AutoUnlockKey>& keys,
                           const char* target_guid,
                           u32 (*attempt)(wchar_t letter, const wchar_t* bek_path, void* user),
                           void* user);
} // namespace detail
#endif // _WIN32

} // namespace bitlocker
} // namespace win
} // namespace cr

#endif // CR_BITLOCKER_H_
