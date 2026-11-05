/* cr_fsck.cpp - the chkdsk launch endpoint, win layer (M-075).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/win/cr_fsck.h"
#include "cr/check/cr_fsck_launch.h"
#include "cr/cr_error.h"
#include "cr/win/cr_elevate.h"
#include <windows.h>

namespace cr {
namespace win {
namespace fsck {

namespace {

using check::fsck_launch::build_chkdsk_command;
using check::fsck_launch::CHKDSK_FIX;
using check::fsck_launch::valid_volume_letter;

bool live_volume_probe(wchar_t letter) {
    wchar_t root[4] = {letter, L':', L'\\', 0};
    const UINT type = GetDriveTypeW(root);
    /* The og's WinChkdsk addresses real volumes; network drives are out
     * (N-013's PathIsNetworkPathW stance - a chkdsk on a UNC volume is
     * meaningless), CD-ROMs have no fixable file system the ladder
     * covers, and DRIVE_NO_ROOT_DIR is the canonical "no volume here". */
    return type == DRIVE_FIXED || type == DRIVE_REMOVABLE;
}

int live_spawn(const std::wstring& cmd, u32 timeout_ms) {
    /* The og's WoW64 guarantee (FileSystemCheck_decompiled.c:7093 ->
     * the x64\WinChkdsk.exe branch :7134): a 32-bit host must launch the
     * NATIVE chkdsk. Disabling the Wow64 redirection around the
     * CreateProcessW makes %SystemRoot%\System32 resolve to the real
     * System32 for both process classes; the reverted state is restored
     * immediately after (the redirection is thread-global). */
    void* redir_old = nullptr;
    const BOOL redir_disabled = Wow64DisableWow64FsRedirection(&redir_old);
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    wchar_t* mutable_cmd = _wcsdup(cmd.c_str());
    if (!mutable_cmd) {
        if (redir_disabled)
            Wow64RevertWow64FsRedirection(redir_old);
        return -1;
    }
    const BOOL created = CreateProcessW(nullptr, mutable_cmd, nullptr, nullptr, FALSE,
                                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    free(mutable_cmd);
    if (redir_disabled)
        Wow64RevertWow64FsRedirection(redir_old);
    if (!created)
        return -1;
    DWORD exit_code = STILL_ACTIVE;
    const ULONGLONG start = GetTickCount64();
    while (GetTickCount64() - start < ULONGLONG(timeout_ms)) {
        if (WaitForSingleObject(pi.hProcess, 500) == WAIT_OBJECT_0)
            break;
    }
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    /* On timeout the process is left to finish on its own (killing
     * chkdsk mid-fix risks the very damage this repairs); the caller
     * sees STILL_ACTIVE (259) and decides. */
    return int(exit_code);
}

std::wstring live_system_root() {
    wchar_t buf[MAX_PATH + 1] = {0};
    const UINT n = GetWindowsDirectoryW(buf, MAX_PATH);
    return std::wstring(buf, n);
}

} // namespace

u32 fsck_volume(wchar_t letter, u32 flags, u32 timeout_ms, int32_t* exit_code) {
    if (exit_code)
        *exit_code = -1;
    if (!valid_volume_letter(letter))
        return ERR_BAD_PARAMETER; /* the loud refusal - never a spawn */
    if (!cr::win::is_elevated())
        return ERR_UNSUPPORTED; /* chkdsk /f needs the volume handle */
    if (!live_volume_probe(letter))
        return ERR_OPEN_FAILED; /* no volume mounted at the letter */
    const std::wstring cmd = build_chkdsk_command(letter, flags, live_system_root());
    const int raw = live_spawn(cmd, timeout_ms);
    if (exit_code)
        *exit_code = int32_t(raw);
    return raw == -1 ? ERR_OPEN_FAILED : OK;
}

#ifdef _WIN32
namespace detail {

u32 fsck_volume_seam(wchar_t letter, u32 flags, u32 timeout_ms, int32_t* exit_code,
                     VolumeProbeFn probe, SpawnFn spawn) {
    if (exit_code)
        *exit_code = -1;
    if (!valid_volume_letter(letter))
        return ERR_BAD_PARAMETER;
    if (!is_elevated())
        return ERR_UNSUPPORTED;
    if (!probe || !probe(letter))
        return ERR_OPEN_FAILED;
    const std::wstring cmd = build_chkdsk_command(letter, flags, live_system_root());
    const int raw = spawn(cmd, timeout_ms);
    if (exit_code)
        *exit_code = int32_t(raw);
    return raw == -1 ? ERR_OPEN_FAILED : OK;
}

} // namespace detail
#endif // _WIN32

} // namespace fsck
} // namespace win
} // namespace cr
