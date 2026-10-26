/* cr_bitlocker.cpp - detect + legitimate unlock only (R8, fix F13).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/win/cr_bitlocker.h"
#include "cr/cr_error.h"
#include "cr/reghive/cr_reghive.h"
#include <windows.h>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace cr {
namespace win {
namespace bitlocker {

namespace {

/* S-014 (diff-to-fix): the og DRWService opens
 * HKLM\SYSTEM\CurrentControlSet\\Control\\FVEAutoUnlock
 * (DRWService_decompiled.c:104, RegOpenKeyExA) - NOT Services\... as the
 * pre-fix core had it. Per-volume subkeys are named by volume GUID and
 * carry a REG_BINARY "Data" blob (the BEK-class external key). */
constexpr wchar_t kFveLivePath[] = L"SYSTEM\\CurrentControlSet\\Control\\FVEAutoUnlock";
constexpr char kFveLivePathA[] = "SYSTEM\\CurrentControlSet\\Control\\FVEAutoUnlock";

/* the og's offline hive location (bitlockerhelp_decompiled.c:3128:
 * snprintf "%s\\Windows\\System32\\config\\SYSTEM") */
constexpr wchar_t kOfflineHiveRel[] = L"\\Windows\\System32\\config\\SYSTEM";

/* hive file read cap - a real SYSTEM hive is 10-100 MiB; 512 MiB is an
 * absurd-but-bounded guard against hostile images */
constexpr u64 kHiveMaxBytes = u64(512) * 1024 * 1024;

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

/* Three-outcome boot probe: the two-state is_fve_volume() cannot distinguish
 * "no FVE signature" from "volume unreadable" (typical for a LOCKED
 * BitLocker volume that denies \\.\X: reads). Misclassifying the latter as
 * clear let scans proceed on locked volumes the og routes to its unlock
 * flow (corpus 04-recovery-workflow §2/§10; AUDIT-FALLBACKS FB-B01). */
namespace {
enum class BootProbe : u8 { Clear, Fve, Unreadable };

BootProbe probe_fve(wchar_t letter) {
    u8 sec[512];
    if (!read_boot_sector(letter, sec))
        return BootProbe::Unreadable;
    /* BitLocker volume boot marker at OEM position (corpus signature) */
    if (std::memcmp(sec + 3, "-FVE-FS-", 8) == 0)
        return BootProbe::Fve;
    return BootProbe::Clear;
}
} // namespace

bool is_fve_volume(wchar_t letter) {
    return probe_fve(letter) == BootProbe::Fve;
}

bitlocker::Status status(wchar_t letter) {
    switch (probe_fve(letter)) {
    case BootProbe::Unreadable:
        /* Never "clear" when the state cannot be determined - the caller
         * must refuse and surface unlock guidance, not scan blind. */
        return Status::Unknown;
    case BootProbe::Clear:
        return Status::Clear;
    case BootProbe::Fve:
        break;
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
    int rc = run_command(detail::build_unlock_command(letter, key_type, key_or_path, false),
                         120000);
    if (rc != 0) {
        /* legacy fallback: manage-bde (corpus pre-PowerShell path) */
        rc = run_command(detail::build_unlock_command(letter, key_type, key_or_path, true),
                         120000);
    }
    if (rc != 0)
        return ERR_FVE_LOCKED;
    return OK;
}

/* N-064 (diff-to-fix): the password-dictionary unlock aid - the og's Dict
 * flow. The EXPLICIT CONSENT ACTION: callers are the host's user-initiated
 * unlock surfaces only (CLI --bitlocker-dict / the cr_ui action); the
 * scan/open paths never reach here. Candidates: the shipped 228-entry
 * dictionary in corpus order, then the user-provided set. Attempts ride
 * the same PowerShell -Password / manage-bde -Password plumbing as a
 * user-typed password (unlock() key_type 0). */
u32 unlock_with_dictionary(wchar_t letter, const wchar_t* const* user_passwords,
                           size_t user_count) {
    std::vector<std::string> user;
    user.reserve(user_count);
    for (size_t i = 0; i < user_count; ++i) {
        if (user_passwords[i] == nullptr)
            continue;
        const wchar_t* p = user_passwords[i];
        std::string narrow;
        for (; *p != L'\0'; ++p) {
            if (*p > 0x7F)
                return ERR_BAD_PARAMETER; /* the dictionary grammar is ASCII */
            narrow.push_back(char(*p));
        }
        if (!narrow.empty() && narrow.size() <= dict::kMaxPasswordLen)
            user.push_back(std::move(narrow));
    }
    const dict::AttemptResult res = detail::dictionary_unlock_seam(
        user,
        [](wchar_t l, const wchar_t* password, void*) -> u32 {
            return unlock(l, 0, password);
        },
        letter, nullptr);
    return res.success_index != 0 ? OK : ERR_FVE_LOCKED;
}

/* The narrow overload: the CLI's loaded dictionary candidates ride the
 * same consent loop (the absorbed convergent-landing capability). */
u32 unlock_with_dictionary(wchar_t letter, const std::vector<std::string>& user_candidates) {
    const dict::AttemptResult res = detail::dictionary_unlock_seam(
        user_candidates,
        [](wchar_t l, const wchar_t* password, void*) -> u32 {
            return unlock(l, 0, password);
        },
        letter, nullptr);
    return res.success_index != 0 ? OK : ERR_FVE_LOCKED;
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

/* S-014: the target volume's GUID ("\??\Volume{...}\" -> "{...}"), for the
 * per-volume record association. Empty when unavailable (removable media
 * without a mount point, etc.) - the caller then just tries records in
 * enumeration order. */
std::string target_volume_guid(wchar_t letter) {
    wchar_t mount[8] = {};
    _snwprintf_s(mount, 8, _TRUNCATE, L"%c:\\", letter);
    wchar_t vol[64] = {};
    if (GetVolumeNameForVolumeMountPointW(mount, vol, 64) == 0)
        return {};
    const wchar_t* lbrace = std::wcschr(vol, L'{');
    const wchar_t* rbrace = std::wcschr(vol, L'}');
    if (lbrace == nullptr || rbrace == nullptr || rbrace < lbrace ||
        size_t(rbrace - lbrace) > cr::reghive::kFveGuidMaxChars)
        return {};
    std::string out;
    for (const wchar_t* p = lbrace; p <= rbrace; ++p)
        out.push_back(char(*p));
    return out;
}

/* the real unlock attempt for apply_auto_unlock_keys (PowerShell .BEK path) */
u32 attempt_bek_unlock(wchar_t letter, const wchar_t* bek_path, void* /*user*/) {
    return unlock(letter, 2, bek_path);
}

/* Read a whole file (bounded) - the offline hive loader. */
bool read_file_bytes(const wchar_t* path, std::vector<u8>& out) {
    HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    LARGE_INTEGER sz = {};
    if (!GetFileSizeEx(h, &sz) || sz.QuadPart <= 0 || u64(sz.QuadPart) > kHiveMaxBytes) {
        CloseHandle(h);
        return false;
    }
    out.resize(size_t(sz.QuadPart));
    DWORD got = 0;
    BOOL ok = TRUE;
    size_t off = 0;
    while (off < out.size() && ok) {
        const size_t remain = out.size() - off;
        const size_t want = remain > 0x100000 ? size_t(0x100000) : remain;
        const DWORD chunk = DWORD(want);
        ok = ReadFile(h, out.data() + off, chunk, &got, nullptr);
        if (got == 0)
            break;
        off += got;
    }
    CloseHandle(h);
    if (!ok || off != out.size()) {
        out.clear();
        return false;
    }
    return true;
}

/* Harvest the FVEAutoUnlock family from an offline hive file (the og's
 * bitlockerhelp flow). Returns false when the file is missing/unreadable
 * or the hive does not validate (the og's RegAPI::OpenHive rejects
 * bad-magic/bad-checksum hives - RegLib_decompiled.c:2376-2377). */
bool harvest_offline_keys(const wchar_t* hive_path, std::vector<reghive::AutoUnlockKey>& out) {
    std::vector<u8> bytes;
    if (!read_file_bytes(hive_path, bytes))
        return false;
    reghive::RegistryHive hive;
    if (!hive.open(bytes.data(), bytes.size()))
        return false; /* not a valid regf hive - the og rejects it too */
    reghive::harvest_fve_auto_unlock(hive, out);
    return true;
}

/* Apply harvested keys against `letter`, matching GUID first. */
u32 apply_keys(wchar_t letter, const std::vector<reghive::AutoUnlockKey>& keys,
               const std::string& target_guid, u32 (*attempt)(wchar_t, const wchar_t*, void*),
               void* user) {
    if (keys.empty())
        return ERR_FVE_LOCKED;
    u32 best = ERR_FVE_LOCKED;
    DWORD index = 0;
    for (const reghive::AutoUnlockKey* k :
         reghive::select_auto_unlock_attempts(keys, target_guid.c_str())) {
        const std::wstring path = write_temp_bek(k->blob.data(), DWORD(k->blob.size()), index++);
        if (path.empty())
            continue;
        const u32 rc = attempt != nullptr ? attempt(letter, path.c_str(), user)
                                          : attempt_bek_unlock(letter, path.c_str(), nullptr);
        DeleteFileW(path.c_str());
        if (rc == OK) {
            best = OK;
            break;
        }
    }
    return best;
}
} // namespace

#ifdef _WIN32
namespace detail {

/* The unlock command builder (N-064 test seam; the command grammar is
 * unchanged from the S-014 landing - PowerShell primary, manage-bde legacy
 * fallback, key material quoted). */
std::wstring build_unlock_command(wchar_t letter, int key_type,
                                  const std::wstring& key_or_path, bool legacy_manage_bde) {
    std::wstring mount;
    mount.push_back(letter);
    mount += L":";
    if (!legacy_manage_bde) {
        std::wstring param;
        if (key_type == 0)
            param = L"-Password ";
        else if (key_type == 1)
            param = L"-RecoveryPassword ";
        else
            param = L"-KeyFilePath "; /* BEK file (external key protector) - corpus path */
        return L"powershell -NoProfile -NonInteractive -Command \"Unlock-BitLocker -MountPoint " +
               quote(mount) + L" " + param + quote(key_or_path) + L"\"";
    }
    std::wstring sub = key_type == 0 ? L"-Password"
                                     : (key_type == 1 ? L"-RecoveryPassword" : L"-KeyFilePath");
    return L"manage-bde -unlock " + mount + L" " + sub + L" " + quote(key_or_path);
}

/* The dictionary loop with the attempt seam: widens each candidate and
 * feeds it to `attempt` (the real flow unlocks; tests record). */
dict::AttemptResult dictionary_unlock_seam(
    const std::vector<std::string>& user_candidates,
    u32 (*attempt)(wchar_t letter, const wchar_t* password, void* user), wchar_t letter,
    void* user) {
    struct Ctx {
        wchar_t letter;
        u32 (*attempt)(wchar_t, const wchar_t*, void*);
        void* user;
    } ctx{letter, attempt, user};
    return dict::try_passwords(
        [](const char* password, void* p) -> u32 {
            Ctx* c = static_cast<Ctx*>(p);
            std::wstring wide;
            for (const char* q = password; *q != '\0'; ++q)
                wide.push_back(static_cast<wchar_t>(static_cast<unsigned char>(*q)));
            return c->attempt(c->letter, wide.c_str(), c->user);
        },
        &ctx, &user_candidates);
}

const char* fve_live_registry_path_a() {
    return kFveLivePathA;
}

/* Enumerate per-volume-GUID "Data" blobs under an OPENED FVEAutoUnlock
 * root. Corpus semantics (DRWService_decompiled.c:104-140): subkeys are
 * volume GUIDs (uppercased like the og's _strupr_s), each carries a
 * REG_BINARY "Data" value; the pipe record caps are 50-char GUID,
 * 256-byte blob, 100 records (artifacts/DRWService-exe--bitlocker-service.md
 * :56-74). */
size_t enum_auto_unlock_keys(void* hkey_root, std::vector<reghive::AutoUnlockKey>& out) {
    HKEY root = reinterpret_cast<HKEY>(hkey_root);
    DWORD subkeys = 0;
    if (RegQueryInfoKeyW(root, nullptr, nullptr, nullptr, &subkeys, nullptr, nullptr, nullptr,
                         nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
        return 0;
    size_t n = 0;
    for (DWORD i = 0; i < subkeys && out.size() < reghive::kFveMaxRecords; ++i) {
        wchar_t name[64] = {};
        DWORD name_len = 64;
        if (RegEnumKeyExW(root, i, name, &name_len, nullptr, nullptr, nullptr, nullptr) !=
            ERROR_SUCCESS)
            continue;
        if (name_len == 0 || name_len > reghive::kFveGuidMaxChars)
            continue; /* corpus cap: 50-char GUID field */
        HKEY sub = nullptr;
        if (RegOpenKeyExW(root, name, 0, KEY_READ, &sub) != ERROR_SUCCESS)
            continue;
        DWORD type = 0;
        DWORD size = 0;
        if (RegQueryValueExW(sub, L"Data", nullptr, &type, nullptr, &size) == ERROR_SUCCESS &&
            type == REG_BINARY && size >= 32) {
            /* the og allocates 0x200 and memcpy's min(size, 0x100) - cap the
             * blob at 256 bytes, floor at 32 (BEK-class blob size) */
            DWORD cap = reghive::kFveMaxBlobBytes;
            if (size < cap)
                cap = size;
            std::vector<u8> buf(cap);
            DWORD got = cap;
            if (RegQueryValueExW(sub, L"Data", nullptr, nullptr, buf.data(), &got) ==
                    ERROR_SUCCESS &&
                got == cap) {
                reghive::AutoUnlockKey key;
                key.volume_guid.reserve(name_len);
                for (DWORD c = 0; c < name_len; ++c) {
                    wchar_t wc = name[c];
                    /* uppercase like the og's _strupr_s (GUID text is ASCII) */
                    if (wc >= L'a' && wc <= L'z')
                        wc = wchar_t(wc - L'a' + L'A');
                    key.volume_guid.push_back(char(wc));
                }
                key.blob = std::move(buf);
                out.push_back(std::move(key));
                ++n;
            }
        }
        RegCloseKey(sub);
    }
    return n;
}

u32 apply_auto_unlock_keys(wchar_t letter, const std::vector<reghive::AutoUnlockKey>& keys,
                           const char* target_guid, u32 (*attempt)(wchar_t, const wchar_t*, void*),
                           void* user) {
    return apply_keys(letter, keys, target_guid != nullptr ? target_guid : "", attempt, user);
}
} // namespace detail
#endif // _WIN32

u32 try_auto_unlock(wchar_t letter) {
    /* Live registry first: the og's DRWService path
     * (SYSTEM\CurrentControlSet\Control\FVEAutoUnlock; S-014 fixed the
     * pre-fix Services\... path). Per-volume-GUID subkeys, "Data" blobs. */
    {
        HKEY k = nullptr;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, kFveLivePath, 0, KEY_READ, &k) == ERROR_SUCCESS) {
            std::vector<reghive::AutoUnlockKey> keys;
            detail::enum_auto_unlock_keys(k, keys);
            RegCloseKey(k);
            const std::string guid = target_volume_guid(letter);
            const u32 rc = apply_keys(letter, keys, guid, nullptr, nullptr);
            if (rc == OK)
                return OK;
        }
    }
    /* Offline probe (the og's bitlockerhelp flow): every OTHER mounted
     * volume's <X>:\Windows\System32\config\SYSTEM hive may hold the key
     * for this volume (dead-system recovery - the flagship scenario). The
     * running OS's letter is skipped (its live registry was just read) and
     * so is the target letter (a locked volume denies the read anyway). */
    {
        wchar_t os_root[8] = L"?:\\";
        wchar_t windir[MAX_PATH] = {};
        if (GetWindowsDirectoryW(windir, MAX_PATH) > 0 && windir[1] == L':')
            os_root[0] = windir[0];
        wchar_t drives[256] = {};
        const DWORD len = GetLogicalDriveStringsW(256, drives);
        const std::string guid = target_volume_guid(letter);
        for (DWORD i = 0; i + 3 < len;) {
            const wchar_t c = drives[i];
            if ((drives[i + 1] == L':' && drives[i + 2] == L'\\') && c != os_root[0] &&
                c != letter && c >= L'A' && c <= L'Z') {
                wchar_t hive[MAX_PATH] = {};
                _snwprintf_s(hive, MAX_PATH, _TRUNCATE, L"%c:%s", c, kOfflineHiveRel);
                if (GetFileAttributesW(hive) != INVALID_FILE_ATTRIBUTES) {
                    std::vector<reghive::AutoUnlockKey> keys;
                    if (harvest_offline_keys(hive, keys)) {
                        const u32 rc = apply_keys(letter, keys, guid, nullptr, nullptr);
                        if (rc == OK)
                            return OK;
                    }
                }
            }
            while (i < len && drives[i] != L'\0')
                ++i;
            ++i; /* skip the NUL separator */
        }
    }
    return ERR_FVE_LOCKED;
}

u32 try_auto_unlock_offline(wchar_t letter, const wchar_t* hive_path) {
    if (hive_path == nullptr || hive_path[0] == L'\0')
        return ERR_FVE_LOCKED;
    std::vector<reghive::AutoUnlockKey> keys;
    if (!harvest_offline_keys(hive_path, keys))
        return ERR_FVE_LOCKED; /* unreadable or invalid hive (og rejects too) */
    const std::string guid = target_volume_guid(letter);
    return apply_keys(letter, keys, guid, nullptr, nullptr);
}

} // namespace bitlocker
} // namespace win
} // namespace cr
