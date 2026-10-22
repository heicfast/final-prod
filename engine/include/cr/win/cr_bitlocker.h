/*
 * cr_bitlocker.h - BitLocker-aware handling (R8).
 *
 * Policy (fix F13): DETECT + LEGITIMATE WINDOWS UNLOCK ONLY. Detection via
 * FVE metadata ("-FVE-FS-" OEM at 0x03, corpus signature). Unlock paths:
 *   1. PowerShell Unlock-BitLocker (password/recovery key) - primary
 *   2. manage-bde -unlock - fallback (corpus legacy path)
 *   3. FVEAutoUnlock registry keys + auto-unlock key application
 * NEVER any decryption-bypass code. Locked volumes report ERR_FVE_LOCKED.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_BITLOCKER_H_
#define CR_BITLOCKER_H_

#include "../cr_types.h"
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

/* Auto-unlock: enumerate FVEAutoUnlock registry values and try them. */
u32 try_auto_unlock(wchar_t letter);

/* Human-readable status for CLI output. */
const wchar_t* status_text(Status s);

} // namespace bitlocker
} // namespace win
} // namespace cr

#endif // CR_BITLOCKER_H_
