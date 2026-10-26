/* cr_bootfix.cpp - the Windows-side boot-chain repair plumbing (M-040
 * stage a's win half + stage b's resolver). Re-implemented from
 * understanding of Fixup.mo / EFIBoot.mo (corpus 03ad46c, gates cited
 * below re-verified first-hand). Never any decompiled text.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/win/cr_bootfix.h"
#include "cr/cr_error.h"
#include "cr/cr_log.h"

#include <windows.h>

#include <cstdio>
#include <cstring>
#include <vector>

namespace cr {
namespace win {
namespace bootfix {

namespace {

/* Fixup :243-255 + EFIBoot :3248-3264: enable a privilege on our own
 * token. Best-effort like the og (a missing privilege logs and the
 * caller decides). */
bool enable_privilege(const wchar_t* name) {
    HANDLE tok = INVALID_HANDLE_VALUE;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tok))
        return false;
    LUID luid;
    if (!LookupPrivilegeValueW(nullptr, name, &luid)) {
        CloseHandle(tok);
        return false;
    }
    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    const BOOL ok = AdjustTokenPrivileges(tok, FALSE, &tp, 0, nullptr, nullptr);
    const DWORD rc = GetLastError();
    CloseHandle(tok);
    return ok != FALSE && rc == ERROR_SUCCESS;
}

/* Fixup :533/:542: ZwUnloadKey(HKEY \Registry\Machine\BCD00000000) -
 * the loaded BCD hive shadows the file being patched. ntdll is
 * resolved dynamically (the og's own XP-compat GetProcAddress pattern,
 * EFIBoot :3281-3290) so no static ntdll import enters the PE gate.
 * Best-effort: on WinPE / dead-system repair the target hive is not
 * loaded and the call fails harmlessly - we log and proceed. */
void unload_bcd_hive_best_effort() {
    using ZwUnloadKeyFn = LONG (*)(void* key);
    HMODULE ntdll = LoadLibraryW(L"ntdll.dll");
    if (ntdll == nullptr)
        return;
    auto fn = reinterpret_cast<ZwUnloadKeyFn>(GetProcAddress(ntdll, "ZwUnloadKey"));
    if (fn == nullptr) {
        FreeLibrary(ntdll);
        return;
    }
    /* HKEY_PERFORMANCE_NLTEXTEXT-style pre-defined handle shapes are
     * not used; the og passes the literal registry handle value for
     * \Registry\Machine\BCD00000000 obtained from a prior open. We open
     * it read-only to obtain the handle, then unload. */
    HKEY key = nullptr;
    const LONG or_ = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"BCD00000000", 0, KEY_READ, &key);
    if (or_ != ERROR_SUCCESS || key == nullptr) {
        FreeLibrary(ntdll);
        return; /* not loaded - the common repair case */
    }
    RegCloseKey(key);
    const LONG rc = fn(key);
    if (rc != 0)
        log::write(log::Level::Info, "[bootfix] BCD hive unload rc=%ld (proceeding)", long(rc));
    FreeLibrary(ntdll);
}

/* OVERLAPPED-positioned write (no SetFilePointer import; the M-009
 * envelope precedent). */
bool write_at(HANDLE h, u64 offset, const void* data, u32 len) {
    OVERLAPPED ov;
    std::memset(&ov, 0, sizeof(ov));
    ov.Offset = DWORD(offset & 0xFFFFFFFFull);
    ov.OffsetHigh = DWORD(offset >> 32);
    DWORD done = 0;
    return WriteFile(h, data, len, &done, &ov) != FALSE && done == len;
}

} // namespace

u32 repair_bcd_file(const wchar_t* bcd_path, u32 mbr_signature, u64 old_start_lba,
                    u64 new_start_lba, bcdfix::ConsentFn consent, void* consent_user,
                    size_t* applied) {
    if (applied != nullptr)
        *applied = 0;
    if (bcd_path == nullptr)
        return ERR_BAD_PARAMETER;

    /* the og's privilege pair for hive surgery (Fixup :243-255) */
    (void)enable_privilege(SE_BACKUP_NAME);
    (void)enable_privilege(SE_RESTORE_NAME);
    unload_bcd_hive_best_effort();

    HANDLE h = CreateFileW(bcd_path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return ERR_OPEN_FAILED;

    LARGE_INTEGER sz;
    if (!GetFileSizeEx(h, &sz) || sz.QuadPart <= 0 || sz.QuadPart > (64ll << 20)) {
        CloseHandle(h);
        return ERR_READ_FAILED;
    }
    std::vector<u8> hive(size_t(sz.QuadPart), 0);
    DWORD got = 0;
    if (!ReadFile(h, hive.data(), DWORD(hive.size()), &got, nullptr) || got != hive.size()) {
        CloseHandle(h);
        return ERR_READ_FAILED;
    }

    const std::vector<bcdfix::BcdRewrite> plan =
        bcdfix::plan_rewrites(hive.data(), hive.size(), mbr_signature, old_start_lba,
                              new_start_lba);
    if (plan.empty()) {
        CloseHandle(h);
        return OK; /* the hive does not reference the partition - the
                    * og's silent no-op */
    }

    /* the P62 consent gate: default-deny (bcdfix's execute_plan model) */
    const bool allowed = consent != nullptr ? consent(consent_user) : false;
    if (!allowed) {
        CloseHandle(h);
        return ERR_REPAIR_REFUSED;
    }

    size_t done = 0;
    u32 rc = OK;
    for (const auto& r : plan) {
        if (!write_at(h, r.field_offset, r.new_bytes, 8)) {
            rc = ERR_WRITE_FAILED;
            break;
        }
        ++done;
    }
    /* the flush tail (Fixup :263-306: the DASD allow + FlushFileBuffers
     * on the system partition - the file-handle flush carries the same
     * guarantee for a file on that volume) */
    if (done != 0)
        (void)FlushFileBuffers(h);
    CloseHandle(h);
    if (applied != nullptr)
        *applied = done;
    return rc;
}

namespace {

/* EFIBoot :3888-3894: the zero-GUID dummy read must fail with something
 * OTHER than ERROR_INVALID_FUNCTION (=1) - the platform has EFI
 * firmware variables (on legacy BIOS the API reports
 * ERROR_INVALID_FUNCTION). */
bool firmware_available(void*) {
    u8 scratch[64];
    SetLastError(0);
    const DWORD n = GetFirmwareEnvironmentVariableW(L"", L"{00000000-0000-0000-0000-000000000000}",
                                                    scratch, sizeof(scratch));
    if (n != 0)
        return true;
    return GetLastError() != ERROR_INVALID_FUNCTION;
}

u32 firmware_get(void* user, const char16_t* name, u8* out, u32 cap) {
    (void)user;
    /* the EFI global variable GUID every og call carries (EFIBoot
     * :3385/:3493/:3518/...) */
    SetLastError(0);
    return GetFirmwareEnvironmentVariableW(reinterpret_cast<const wchar_t*>(name),
                                           L"{8BE4DF61-93CA-11d2-AA0D-00E098032B8C}", out, cap);
}

bool firmware_set(void* user, const char16_t* name, const u8* data, u32 len) {
    (void)user;
    SetLastError(0);
    return SetFirmwareEnvironmentVariableW(reinterpret_cast<const wchar_t*>(name),
                                           L"{8BE4DF61-93CA-11d2-AA0D-00E098032B8C}", data,
                                           len) != FALSE;
}

} // namespace

efifix::FirmwareVars make_firmware_vars() {
    efifix::FirmwareVars fw;
    fw.available = firmware_available;
    fw.get = firmware_get;
    fw.set = firmware_set;
    fw.user = nullptr;
    /* EFIBoot :3248-3264: SeSystemEnvironmentPrivilege before any
     * firmware write. A failed enable keeps the vtable complete - the
     * og proceeds and the API fails per-call - but we log it. */
    if (!enable_privilege(SE_SYSTEM_ENVIRONMENT_NAME))
        log::write(log::Level::Warn, "[bootfix] SeSystemEnvironmentPrivilege not enabled");
    return fw;
}

u32 run_boot_chain_repair(const wchar_t* const* bcd_paths, size_t bcd_path_count,
                          u32 mbr_signature, u64 old_start_lba, u64 new_start_lba,
                          const efifix::PartitionRef& nvram_old,
                          const efifix::PartitionRef& nvram_new, bcdfix::ConsentFn consent,
                          void* consent_user, BootRepairResult* out) {
    BootRepairResult r;
    u32 worst = OK;

    for (size_t i = 0; i < bcd_path_count; ++i) {
        size_t cells = 0;
        const u32 rc = repair_bcd_file(bcd_paths[i], mbr_signature, old_start_lba, new_start_lba,
                                       consent, consent_user, &cells);
        if (rc != OK && r.bcd_rc == OK)
            r.bcd_rc = rc;
        if (rc == OK && cells != 0)
            ++r.bcd_files_patched;
        r.bcd_cells_patched += cells;
        if (rc != OK)
            worst = rc; /* reported, does not block the NVRAM leg (the
                         * og's chain continues past a Bcd failure) */
    }

    const efifix::FirmwareVars fw = make_firmware_vars();
    r.nvram_rc = efifix::update_boot_entries(fw, nvram_old, nvram_new, consent, consent_user,
                                             &r.nvram);
    if (r.nvram_rc != OK && worst == OK)
        worst = r.nvram_rc;

    if (out != nullptr)
        *out = r;
    return worst;
}

} // namespace bootfix
} // namespace win
} // namespace cr
