/*
 * cr_fsck_launch.h - the check-phase escalation endpoint (M-075, diff-to-fix).
 *
 * Corpus contract (the escalation ladder, rawfixer-fs-repair.md:59-65 +
 * the M-075 finding's citations, re-verified at corpus be0cfe9):
 *   FileSystemCheck.mo detects damage in-engine (the M-041 layer) and the
 *   og escalates to WinChkdsk.exe - a console wrapper that launches
 *   \System32 chkdsk itself (per-binary/WinChkdsk.md:28 "Runs real chkdsk
 *   (\System32\ + chkdsk invocation), config via WinChkdsk.ini, results
 *   via LogMsg.txt/LogVss.txt"). The launch site:
 *   FileSystemCheck_decompiled.c:7099 (memcpy "WinChkdsk.exe"), :7134
 *   ("x64\\WinChkdsk.exe" - the WoW64 branch behind IsWow64Process at
 *   :7093), :7217 (CreateProcessA). The user-facing guidance string:
 *   strings_ascii.txt:126 "The volume have some errors,please use Chkdsk
 *   to check, and use Chkdsk /F to fix!" - TWO postures: check (no /f)
 *   and fix (/f).
 *
 * The og's exact 5 arguments to WinChkdsk.exe are NOT decoded (the corpus
 * names the count, not the values - M-075's open question). This core
 * therefore builds the DIRECT chkdsk.exe form (what WinChkdsk itself
 * wraps): "<system_root>\System32\chkdsk.exe X: /f" - the corpus-grounded
 * base the og's wrapper delegates to. If a future decode recovers the 5
 * arguments, reconcile against this form per the finding's fix plan.
 *
 * The WoW64 branch the og implements by launching x64\WinChkdsk.exe is
 * realized at the win layer by disabling Wow64 filesystem redirection
 * around the CreateProcessW (the native System32 resolves for both
 * process classes); the pure builder stays bitness-agnostic.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_FSCK_LAUNCH_H_
#define CR_FSCK_LAUNCH_H_

#include "../cr_types.h"
#include <string>

namespace cr {
namespace check {
namespace fsck_launch {

/* The two postures of the guidance string (strings_ascii.txt:126):
 * check the volume, or fix it. No other flag is invented - the corpus
 * does not decode more of the wrapper's chkdsk argument set. */
enum ChkdskFlags : u32 {
    CHKDSK_SCAN_ONLY = 0, /* "use Chkdsk to check" - no /f */
    CHKDSK_FIX = 1u << 0, /* "use Chkdsk /F to fix" - appends /f */
};

/* True when `letter` is a volume letter the launcher can address
 * ('A'..'Z' / 'a'..'z' - Windows volume letters; anything else,
 * including digits, UNC paths and NUL, is refused loudly upstream). */
bool valid_volume_letter(wchar_t letter);

/* The command line the launcher spawns:
 *   "<system_root>\System32\chkdsk.exe <letter>: [/f]"
 * `system_root` is passed in (the win layer supplies the live
 * GetWindowsDirectoryW value; tests pin their own) so the builder is
 * pure and cross-platform - the N-064 build_unlock_command seam
 * precedent. The volume letter is uppercased exactly like the og's
 * volume addressing (one char, no quoting needed - it cannot carry a
 * space or a quote). Unknown flag bits are ignored (forward
 * compatibility: the host may pass future bits; only /f is wired). */
std::wstring build_chkdsk_command(wchar_t letter, u32 flags, const std::wstring& system_root);

/* The documented Windows chkdsk exit codes (Microsoft's documented OS
 * behavior, not corpus-decoded - the corpus's results flow through
 * WinChkdsk's LogMsg.txt, which no finding decodes). Returned raw
 * through the ABI's int32_t; this helper exists so hosts and tests name
 * the classes instead of magic numbers:
 *   0 - no file system errors found
 *   1 - errors found and fixed
 *   2 - disk cleanup performed (or not needed) / offline scan scheduled
 *   3 - errors found and NOT fixed (the volume needs the /f posture)
 * 259 - STILL_ACTIVE: the launch timed out (the process was left to
 *       finish on its own - killing chkdsk mid-fix is never safe) */
bool chkdsk_exit_means_errors_remain(int32_t exit_code);

} // namespace fsck_launch
} // namespace check
} // namespace cr

#endif // CR_FSCK_LAUNCH_H_
