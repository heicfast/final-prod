/* cr_bootfix.cpp - the Windows side of the boot-chain repair (M-040
 * stages a+b). See include/cr/win/cr_bootfix.h for the corpus lineage
 * and the safety model.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/win/cr_bootfix.h"
#include "cr/cr_error.h"
#include <windows.h>
#include <cstring>
#include <cstdio>

namespace cr {
namespace win {
namespace bootfix {

/* forward: used by open_hive's force-unload retry (defined below) */
bool force_unload_bcd_hive();

namespace {

/* ntdll entry points, dynamically resolved (the og's own posture:
 * nothing lands on the static import table - the Win7 allowlist gate
 * stays untouched). */
using RtlAdjustPrivilegeFn = LONG(WINAPI*)(unsigned long, int, int, unsigned char*);
using ZwUnloadKeyFn = LONG(WINAPI*)(const void*);

/* privilege LUIDs (the well-known set; SeBackup 17 / SeRestore 18 /
 * SeSystemEnvironment 22 - the corpus enables exactly these) */
constexpr unsigned long kSeBackup = 17;
constexpr unsigned long kSeRestore = 18;
constexpr unsigned long kSeSystemEnvironment = 22;

/* documented NT status values */
constexpr LONG kStatusSuccess = 0;
constexpr LONG kStatusObjectNameNotFound = 0xC0000034L;

/* ---- the documented NT structures for ZwUnloadKey (public DDK
 * knowledge, the og's own Fixup :533/:542 call shape) ---- */
struct NtUnicodeString {
    unsigned short length; /* bytes, no NUL */
    unsigned short maximum_length;
    wchar_t* buffer;
};
struct NtObjectAttributes {
    unsigned long length;
    void* root_directory;
    NtUnicodeString* object_name;
    unsigned long attributes;
    void* security_descriptor;
    void* security_quality_of_service;
};

RtlAdjustPrivilegeFn rtl_adjust_privilege() {
    const HMODULE nt = GetModuleHandleW(L"ntdll.dll");
    return nt ? reinterpret_cast<RtlAdjustPrivilegeFn>(
                    reinterpret_cast<void*>(GetProcAddress(nt, "RtlAdjustPrivilege")))
              : nullptr;
}

bool adjust_privilege(unsigned long privilege, bool enable) {
    const RtlAdjustPrivilegeFn fn = rtl_adjust_privilege();
    if (fn == nullptr)
        return false;
    unsigned char prev = 0;
    return fn(privilege, enable ? 1 : 0, 0, &prev) == kStatusSuccess;
}

/* the firmware seam (EFIBoot :3281-3290: GetProcAddress on kernel32 -
 * an XP-era box without the exports still loads) */
struct FirmwareSeam {
    unsigned long(WINAPI* get)(const wchar_t*, void*, unsigned long);
    bool(WINAPI* set)(const wchar_t*, const void*, unsigned long);
    bool resolved = false;
};

const FirmwareSeam& firmware_seam() {
    static FirmwareSeam seam = [] {
        FirmwareSeam s;
        const HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
        if (k32 != nullptr) {
            s.get = reinterpret_cast<unsigned long(WINAPI*)(const wchar_t*, void*, unsigned long)>(
                reinterpret_cast<void*>(GetProcAddress(k32, "GetFirmwareEnvironmentVariableW")));
            s.set = reinterpret_cast<bool(WINAPI*)(const wchar_t*, const void*, unsigned long)>(
                reinterpret_cast<void*>(GetProcAddress(k32, "SetFirmwareEnvironmentVariableW")));
        }
        s.resolved = s.get != nullptr && s.set != nullptr;
        return s;
    }();
    return seam;
}

/* positional read/write through OVERLAPPED (the codebase's file-IO
 * discipline - no SetFilePointer anywhere) */
bool read_whole(HANDLE h, std::vector<u8>* out) {
    LARGE_INTEGER sz{};
    if (!GetFileSizeEx(h, &sz) || sz.QuadPart <= 0 || sz.QuadPart > (256ll << 20))
        return false; /* absent or absurd: a real BCD is < 1 MiB */
    out->resize(size_t(sz.QuadPart));
    size_t done = 0;
    while (done < out->size()) {
        OVERLAPPED ov{};
        ov.Offset = u32((u64(done)) & 0xFFFFFFFFull);
        ov.OffsetHigh = u32((u64(done)) >> 32);
        DWORD got = 0;
        const DWORD chunk =
            DWORD((out->size() - done) > (1u << 20) ? (1u << 20) : (out->size() - done));
        if (!ReadFile(h, out->data() + done, chunk, &got, &ov) || got == 0)
            return false;
        done += got;
    }
    return true;
}

bool write_at(HANDLE h, u64 at, const void* data, u32 len) {
    OVERLAPPED ov{};
    ov.Offset = u32(at & 0xFFFFFFFFull);
    ov.OffsetHigh = u32(at >> 32);
    DWORD put = 0;
    return WriteFile(h, data, len, &put, &ov) && put == len;
}

/* open the hive for read (share-read) or read/write (exclusive; the
 * write open retries once after the force-unload) */
HANDLE open_hive(const wchar_t* path, bool writable) {
    HANDLE h = CreateFileW(path, writable ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ,
                           writable ? 0u : FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h != INVALID_HANDLE_VALUE || !writable)
        return h;
    if (GetLastError() != ERROR_SHARING_VIOLATION)
        return h;
    /* the registry loader holds the file: the og's force-unload + retry
     * (Fixup :533/:542 - ZwUnloadKey under SeRestore/SeBackup :243-255) */
    if (!force_unload_bcd_hive())
        return h;
    SetLastError(ERROR_SHARING_VIOLATION);
    return CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0u, nullptr, OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL, nullptr);
}

} // namespace

/* ---- the force-unload (exposed; see header) ---- */
bool force_unload_bcd_hive() {
    const HMODULE nt = GetModuleHandleW(L"ntdll.dll");
    if (nt == nullptr)
        return false;
    const ZwUnloadKeyFn unload =
        reinterpret_cast<ZwUnloadKeyFn>(reinterpret_cast<void*>(GetProcAddress(nt, "ZwUnloadKey")));
    if (unload == nullptr)
        return false;
    /* the og enables both before the unload (Fixup :243-255) */
    const bool had_restore = adjust_privilege(kSeRestore, true);
    const bool had_backup = adjust_privilege(kSeBackup, true);
    if (!had_restore && !had_backup)
        return false; /* no privilege: a clean refusal, not an error */
    static const wchar_t kName[] = L"\\Registry\\Machine\\BCD00000000";
    NtUnicodeString us{};
    us.length = sizeof(kName) - sizeof(wchar_t);
    us.maximum_length = sizeof(kName);
    us.buffer = const_cast<wchar_t*>(kName);
    NtObjectAttributes oa{};
    oa.length = sizeof(oa);
    oa.object_name = &us;
    const LONG st = unload(&oa);
    /* not-loaded is success for our purpose; privileges restored best
     * effort (the process is short-lived, the og never restores either) */
    return st == kStatusSuccess || st == kStatusObjectNameNotFound;
}

/* ---- stage a ---- */
u32 bcd_file_plan(const wchar_t* path, u32 mbr_signature, u64 old_start_lba, u64 new_start_lba,
                  std::vector<bcdfix::BcdRewrite>* out, unsigned long* last_error) {
    if (last_error)
        *last_error = 0;
    if (out == nullptr)
        return ERR_BAD_PARAMETER;
    out->clear();
    HANDLE h = open_hive(path, false);
    if (h == INVALID_HANDLE_VALUE) {
        if (last_error)
            *last_error = GetLastError();
        return ERR_READ_FAILED;
    }
    std::vector<u8> hive;
    bool ok = read_whole(h, &hive);
    const unsigned long le = ok ? 0 : GetLastError();
    CloseHandle(h);
    if (!ok) {
        if (last_error)
            *last_error = le;
        return ERR_READ_FAILED;
    }
    *out = bcdfix::plan_rewrites(hive.data(), hive.size(), mbr_signature, old_start_lba,
                                 new_start_lba);
    return OK;
}

namespace {
u32 bcd_file_apply(const wchar_t* path, const std::vector<bcdfix::BcdRewrite>& plan, bool new_bytes,
                   bcdfix::ConsentFn consent, void* consent_user, size_t* applied,
                   unsigned long* last_error, bool need_consent) {
    if (applied)
        *applied = 0;
    if (last_error)
        *last_error = 0;
    if (plan.empty())
        return OK;
    if (need_consent && (consent == nullptr || !consent(consent_user)))
        return ERR_REPAIR_REFUSED; /* P62 default-deny: zero writes */
    HANDLE h = open_hive(path, true);
    if (h == INVALID_HANDLE_VALUE) {
        if (last_error)
            *last_error = GetLastError();
        return ERR_READ_FAILED; /* open failed - nothing written */
    }
    u32 rc = OK;
    for (const auto& r : plan) {
        if (!write_at(h, r.field_offset, new_bytes ? r.new_bytes : r.old_bytes, 8)) {
            rc = ERR_WRITE_FAILED;
            break;
        }
        if (applied)
            ++*applied;
    }
    if (rc == OK && !FlushFileBuffers(h)) {
        rc = ERR_WRITE_FAILED; /* the og's post-patch durability tail */
        if (last_error)
            *last_error = GetLastError();
    } else if (rc != OK && last_error) {
        *last_error = GetLastError();
    }
    CloseHandle(h);
    return rc;
}
} // namespace

u32 bcd_file_execute(const wchar_t* path, const std::vector<bcdfix::BcdRewrite>& plan,
                     bcdfix::ConsentFn consent, void* consent_user, size_t* applied,
                     unsigned long* last_error) {
    return bcd_file_apply(path, plan, /*new_bytes=*/true, consent, consent_user, applied,
                          last_error, /*need_consent=*/true);
}

u32 bcd_file_undo(const wchar_t* path, const std::vector<bcdfix::BcdRewrite>& plan, size_t* applied,
                  unsigned long* last_error) {
    /* undo is the recovery path - it runs WITHOUT the consent gate
     * (the consent authorized the transaction; undo restores it) */
    return bcd_file_apply(path, plan, /*new_bytes=*/false, nullptr, nullptr, applied, last_error,
                          /*need_consent=*/false);
}

/* ---- stage b ---- */
bool firmware_seam_available() {
    return firmware_seam().resolved;
}

bool enable_system_environment_privilege() {
    return adjust_privilege(kSeSystemEnvironment, true);
}

namespace {
u32 __nvram_get(const char16_t* name, void* buf, u32 cap, void* /*user*/) {
    const FirmwareSeam& s = firmware_seam();
    if (!s.resolved)
        return 0;
    return s.get(reinterpret_cast<const wchar_t*>(name), buf, cap);
}
bool __nvram_set(const char16_t* name, const void* data, u32 len, void* /*user*/) {
    const FirmwareSeam& s = firmware_seam();
    if (!s.resolved)
        return false;
    return s.set(reinterpret_cast<const wchar_t*>(name), data, len);
}
} // namespace

std::vector<nvramfix::NvramRewrite> nvram_plan(const nvramfix::TargetPartition& target,
                                                u32* error, bool* stopped_early) {
    if (stopped_early)
        *stopped_early = false;
    if (!firmware_seam().resolved) {
        if (error)
            *error = ERR_UNSUPPORTED;
        return {};
    }
    if (!enable_system_environment_privilege()) {
        if (error)
            *error = ERR_REPAIR_REFUSED; /* no privilege: refuse cleanly */
        return {};
    }
    return nvramfix::plan_boot_entries(&__nvram_get, nullptr, target, error, stopped_early);
}

u32 nvram_execute(const std::vector<nvramfix::NvramRewrite>& plan, nvramfix::ConsentFn consent,
                  void* consent_user, size_t* applied) {
    if (!firmware_seam().resolved)
        return ERR_UNSUPPORTED;
    return nvramfix::execute_plan(plan, &__nvram_set, nullptr, consent, consent_user, applied);
}

u32 nvram_undo(const std::vector<nvramfix::NvramRewrite>& plan, size_t* applied) {
    if (!firmware_seam().resolved)
        return ERR_UNSUPPORTED;
    return nvramfix::undo_plan(plan, &__nvram_set, nullptr, applied);
}

} // namespace bootfix
} // namespace win
} // namespace cr
