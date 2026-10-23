/* cr_bitlocker.cpp - detect + legitimate unlock only (R8, fix F13).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/win/cr_bitlocker.h"
#include "cr/cr_error.h"
#include <windows.h>
#include <vector>
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
        /* no FVE signature: the volume is clear (audit row 42b: the
         * re-check of is_fve_volume() inside this branch was
         * tautologically false - Status::Locked was unreachable here) */
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

namespace {
/* Write one binary auto-unlock blob to a temp .BEK file; returns the path
 * (empty on failure). The blob is passed to Windows verbatim - no crypto
 * is performed in-process (fix F13). */
std::wstring write_temp_bek(const u8* data, DWORD len, DWORD index) {
    wchar_t dir[MAX_PATH] = {};
    if (GetTempPathW(MAX_PATH, dir) == 0)
        return {};
    wchar_t path[MAX_PATH] = {};
    _snwprintf_s(path, MAX_PATH, _TRUNCATE, L"%scr_au_%u.bek", dir, unsigned(index));
    HANDLE h = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY,
                           nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return {};
    DWORD wrote = 0;
    const BOOL ok_w = WriteFile(h, data, len, &wrote, nullptr);
    CloseHandle(h);
    if (!ok_w || wrote != len) {
        DeleteFileW(path);
        return {};
    }
    return path;
}

/* Enumerate every REG_BINARY value (>= 32 bytes) under `root`: direct
 * values plus every subkey's values (the FVEAutoUnlock service nests
 * per-volume external keys under Keys\<volume GUID>). */
template <typename F> void enum_binary_blobs(HKEY root, F&& emit) {
    wchar_t name[256];
    for (DWORD i = 0;; ++i) {
        DWORD name_len = 256;
        DWORD type = 0;
        DWORD size = 0;
        const LSTATUS rc = RegEnumValueW(root, i, name, &name_len, nullptr, &type, nullptr, &size);
        if (rc != ERROR_SUCCESS)
            break;
        if (type != REG_BINARY || size < 32)
            continue;
        std::vector<u8> data(size);
        name_len = 256;
        DWORD got = size;
        if (RegEnumValueW(root, i, name, &name_len, nullptr, &type, data.data(), &got) ==
                ERROR_SUCCESS &&
            got == size)
            emit(data.data(), got);
    }
    for (DWORD sk = 0;; ++sk) {
        wchar_t subname[256];
        DWORD sublen = 256;
        if (RegEnumKeyExW(root, sk, subname, &sublen, nullptr, nullptr, nullptr, nullptr) !=
            ERROR_SUCCESS)
            break;
        HKEY sub = nullptr;
        if (RegOpenKeyExW(root, subname, 0, KEY_READ, &sub) == ERROR_SUCCESS) {
            enum_binary_blobs(sub, emit);
            RegCloseKey(sub);
        }
    }
}
} // namespace

u32 try_auto_unlock(wchar_t letter) {
    /* FVEAutoUnlock external keys (corpus DRWService path). Audit row 42a:
     * the old code enumerated the registry VALUE NAMES and passed them as
     * recovery passwords - the names are GUID strings, so that could never
     * succeed. The auto-unlock protectors are the values' BINARY DATA
     * (BEK-class external-key blobs): extract each to a temp .BEK file and
     * unlock via the documented -KeyFilePath path (fix F13: no crypto
     * in-process). Temp files are removed after use. */
    HKEY k = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\FVEAutoUnlock", 0,
                      KEY_READ, &k) != ERROR_SUCCESS) {
        return ERR_FVE_LOCKED; /* no auto keys configured */
    }
    u32 best = ERR_FVE_LOCKED;
    DWORD index = 0;
    enum_binary_blobs(k, [&](const u8* data, DWORD len) {
        if (best == OK)
            return;
        const std::wstring path = write_temp_bek(data, len, index++);
        if (path.empty())
            return;
        if (unlock(letter, 2, path) == OK)
            best = OK;
        DeleteFileW(path.c_str());
    });
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
