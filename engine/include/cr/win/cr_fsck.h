/*
 * cr_fsck.h - the chkdsk launch endpoint, win layer (M-075, diff-to-fix).
 *
 * The og's escalation ladder: FileSystemCheck.mo (in-engine detection,
 * the M-041 layer) -> WinChkdsk.exe (the wrapper, corpus
 * per-binary/WinChkdsk.md:28) -> real \System32 chkdsk. This layer is
 * the core's third rung: launch chkdsk.exe on a volume, elevated,
 * synchronous with a timeout, with the corpus's WoW64 guarantee (the
 * og's IsWow64Process branch at FileSystemCheck_decompiled.c:7093 ->
 * "x64\\WinChkdsk.exe" :7134 - a native-binary launch from a 32-bit
 * host; our form disables Wow64 filesystem redirection around the
 * CreateProcessW so %SystemRoot%\System32\chkdsk.exe always resolves
 * to the native binary).
 *
 * Safety stance (the loud refusal - never a silent skip): a letter that
 * is not a mounted volume, or a caller without elevation, gets an
 * ERR_* code and nothing is spawned.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_WIN_FSCK_H_
#define CR_WIN_FSCK_H_

#include "../cr_types.h"
#include <string>

namespace cr {
namespace win {
namespace fsck {

/* Launch chkdsk on the volume mounted at `letter`.
 *   flags - cr::check::fsck_launch::ChkdskFlags (CHKDSK_FIX appends /f)
 *   timeout_ms - the synchronous wait bound; on timeout the process is
 *     LEFT RUNNING (killing chkdsk mid-fix is never safe) and
 *     *exit_code receives STILL_ACTIVE (259)
 *   exit_code - the raw chkdsk exit code (or -1 when the spawn itself
 *     failed; the documented classes are in cr_fsck_launch.h)
 * Returns OK when the launch ran (chkdsk's own verdict is in
 * *exit_code), ERR_BAD_PARAMETER when the letter is not a volume
 * letter, ERR_OPEN_FAILED when no volume is mounted at the letter (the
 * loud refusal), ERR_UNSUPPORTED when the caller is not elevated. */
u32 fsck_volume(wchar_t letter, u32 flags, u32 timeout_ms, int32_t* exit_code);

#ifdef _WIN32
namespace detail {
/* ---- test seams (Windows lane; test_fsck_win.cpp) ------------------- */

/* The spawn seam: run `cmd` and report its exit code under the timeout
 * bound. The real implementation is the CreateProcessW + wait loop (the
 * cr_bitlocker run_command shape, plus the WoW64 redirection disable);
 * tests inject a recorder to pin the wiring without spawning chkdsk. */
using SpawnFn = int (*)(const std::wstring& cmd, u32 timeout_ms);

/* The volume probe seam: true when a volume is mounted at `letter`.
 * The real implementation probes GetDriveTypeW; tests inject. */
using VolumeProbeFn = bool (*)(wchar_t letter);

/* The full flow with every seam injected (fsck_volume is this with the
 * live implementations pinned). Returns the same codes as fsck_volume
 * and fills *exit_code. */
u32 fsck_volume_seam(wchar_t letter, u32 flags, u32 timeout_ms, int32_t* exit_code,
                     VolumeProbeFn probe, SpawnFn spawn);
} // namespace detail
#endif // _WIN32

} // namespace fsck
} // namespace win
} // namespace cr

#endif // CR_WIN_FSCK_H_
