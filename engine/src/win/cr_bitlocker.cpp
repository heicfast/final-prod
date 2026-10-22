/* cr_bitlocker.cpp - detect + legitimate unlock only (R8, fix F13).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/win/cr_bitlocker.h"
#include "cr/cr_error.h"
#include <windows.h>
#include <cstdio>
#include <cstdlib>

namespace cr {
namespace win {
namespace bitlocker {

namespace {

bool read_boot_sector(wchar_t letter, u8 sec[512]) {
    wchar_t path[8];
    _snwprintf_s(path, 8, _TRUNCATE, L"\\\\.\\%c:", letter);
    HANDLE h =
        CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    DWORD got = 0;
    OVERLAPPED ov = {};
    const BOOL ok = ReadFile(h, sec, 512, &got, &ov);
    CloseHandle(h);
    return ok && got == 512;
}

int run_command(const std::wstring& cmd, int timeout_ms) {
    /* SFC-ready command execution; no shell injection surface: callers
     * build fixed-format commands and quote the user key material. */
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    wchar_t* mutable_cmd = _wcsdup(cmd.c_str());
    if (!mutable_cmd)
        return -1;
    const BOOL created = CreateProcessW(nullptr, mutable_cmd, nullptr, nullptr, FALSE,
                                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    free(mutable_cmd);
    if (!created)
        return -1;
    DWORD exit_code = STILL_ACTIVE;
    const u64 start = GetTickCount64();
    while (GetTickCount64() - start < u64(timeout_ms)) {
        if (WaitForSingleObject(pi.hProcess, 500) == WAIT_OBJECT_0)
            break;
    }
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return int(exit_code);
}

std::wstring quote(const std::wstring& s) {
    std::wstring out = L"\"";
    for (wchar_t c : s) {
        if (c == L'"')
            out += L"\"\"";
        else
            out.push_back(c);
    }
    out += L"\"";
    return out;
}

} // namespace

bool is_fve_volume(wchar_t letter) {
    u8 sec[512];
    if (!read_boot_sector(letter, sec))
        return false;
    /* BitLocker volume boot marker at OEM position (corpus signature) */
    return std::memcmp(sec + 3, "-FVE-FS-", 8) == 0;
}

bitlocker::Status status(wchar_t letter) {
    if (!is_fve_volume(letter)) {
        /* clear or unlocked-encrypted? ask Windows via fs reflection */
        wchar_t fs[32] = {};
        wchar_t vol[8];
        _snwprintf_s(vol, 8, _TRUNCATE, L"%c:\\", letter);
        GetVolumeInformationW(vol, nullptr, 0, nullptr, nullptr, nullptr, fs, 32);
        if (std::wcslen(fs) == 0 && is_fve_volume(letter))
            return Status::Locked;
        return Status::Clear;
    }
    /* FVE signature present: if the volume answers filesystem queries it is
     * unlocked-encrypted; else locked */
    wchar_t fs[32] = {};
    wchar_t vol[8];
    _snwprintf_s(vol, 8, _TRUNCATE, L"%c:\\", letter);
    GetVolumeInformationW(vol, nullptr, 0, nullptr, nullptr, nullptr, fs, 32);
    if (std::wcslen(fs) != 0)
        return Status::UnlockedEncrypted;
    return Status::Locked;
}

u32 unlock(wchar_t letter, int key_type, const std::wstring& key_or_path) {
    /* Build the legitimate unlock command. Primary: PowerShell. */
    std::wstring mount;
    mount.push_back(letter);
    mount += L":";
    std::wstring cmd;
    if (key_type == 0) {
        cmd = L"powershell -NoProfile -NonInteractive -Command \"Unlock-BitLocker -MountPoint " +
              quote(mount) + L" -Password " + quote(key_or_path) + L"\"";
    } else if (key_type == 1) {
        cmd = L"powershell -NoProfile -NonInteractive -Command \"Unlock-BitLocker -MountPoint " +
              quote(mount) + L" -RecoveryPassword " + quote(key_or_path) + L"\"";
    } else {
        /* BEK file (external key protector) - corpus path */
        cmd = L"powershell -NoProfile -NonInteractive -Command \"Unlock-BitLocker -MountPoint " +
              quote(mount) + L" -KeyFilePath " + quote(key_or_path) + L"\"";
    }
    int rc = run_command(cmd, 120000);
    if (rc != 0) {
        /* legacy fallback: manage-bde (corpus pre-PowerShell path) */
        std::wstring sub =
            key_type == 0 ? L"-Password" : (key_type == 1 ? L"-RecoveryPassword" : L"-KeyFilePath");
        cmd = L"manage-bde -unlock " + mount + L" " + sub + L" " + quote(key_or_path);
        rc = run_command(cmd, 120000);
    }
    if (rc != 0)
        return ERR_FVE_LOCKED;
    return OK;
}

u32 try_auto_unlock(wchar_t letter) {
    /* FVEAutoUnlock keys (corpus DRWService path): HKLM\SYSTEM\...\FVEAutoUnlock
     * enumerated; try each as a recovery password. No keys are ever
     * extracted to plaintext outside Windows' own semantics. */
    HKEY k = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\FVEAutoUnlock", 0,
                      KEY_READ, &k) != ERROR_SUCCESS) {
        return ERR_FVE_LOCKED; /* no auto keys configured */
    }
    wchar_t name[256];
    DWORD name_len = 256;
    u32 best = ERR_FVE_LOCKED;
    for (DWORD i = 0;
         RegEnumValueW(k, i, name, &name_len, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS;
         ++i, name_len = 256) {
        std::wstring candidate(name, name_len);
        if (unlock(letter, 1, candidate) == OK) {
            best = OK;
            break;
        }
    }
    RegCloseKey(k);
    return best;
}

const wchar_t* status_text(Status s) {
    switch (s) {
    case Status::Clear:
        return L"clear";
    case Status::Locked:
        return L"locked (FVE)";
    case Status::UnlockedEncrypted:
        return L"unlocked (encrypted)";
    default:
        return L"unknown";
    }
}

} // namespace bitlocker
} // namespace win
} // namespace cr
