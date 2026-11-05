/* cr_fsck_launch.cpp - the pure escalation-endpoint grammar (M-075).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/check/cr_fsck_launch.h"

namespace cr {
namespace check {
namespace fsck_launch {

bool valid_volume_letter(wchar_t letter) {
    return (letter >= L'A' && letter <= L'Z') || (letter >= L'a' && letter <= L'z');
}

std::wstring build_chkdsk_command(wchar_t letter, u32 flags, const std::wstring& system_root) {
    wchar_t upper = valid_volume_letter(letter) ? letter : L'?';
    if (upper >= L'a' && upper <= L'z')
        upper = wchar_t(upper - L'a' + L'A');
    std::wstring cmd = system_root;
    if (!cmd.empty() && cmd.back() != L'\\' && cmd.back() != L'/')
        cmd += L'\\';
    cmd += L"System32\\chkdsk.exe ";
    cmd += upper;
    cmd += L':';
    if (flags & CHKDSK_FIX)
        cmd += L" /f";
    return cmd;
}

bool chkdsk_exit_means_errors_remain(int32_t exit_code) {
    /* Windows-documented classes only: 0 clean, 1 fixed, 2 cleanup or
     * scheduled - the host re-runs /f when the volume still reports
     * damage. 259 (STILL_ACTIVE) is the timeout, not a verdict. */
    return exit_code < 0 || (exit_code >= 3 && exit_code != 259);
}

} // namespace fsck_launch
} // namespace check
} // namespace cr
