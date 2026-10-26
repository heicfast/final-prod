/* cr_bootfix.cpp - the post-recovery boot-chain repair family's win
 * runtime (M-040: the plumbing under cr/bcdfix + cr/nvramfix; every
 * mechanism documented in cr_win_bootfix.h's corpus citations).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/win/cr_bootfix.h"
#include "cr/cr_error.h"
#include <windows.h>
#include <cstdio>
#include <vector>

namespace cr {
namespace win {
namespace bootfix {

namespace {

/* ---- privilege enablement (Fixup :527-546 / EFIBoot :3248-3270) ---- */

bool enable_privilege(const wchar_t* name) {
    HANDLE tok = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tok))
        return false;
    LUID luid = {};
    if (!LookupPrivilegeValueW(nullptr, name, &luid)) {
        CloseHandle(tok);
        return false;
    }
    TOKEN_PRIVILEGES tp = {};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    const BOOL ok = AdjustTokenPrivileges(tok, FALSE, &tp, sizeof(tp), nullptr, nullptr);
    CloseHandle(tok);
    return ok != FALSE && GetLastError() == ERROR_SUCCESS;
}

/* Map a GetFirmwareEnvironmentVariableW failure's GetLastError() to the
 * store contract: not-found (2) is the benign absent case; invalid
 * function (1) means the API does not exist (BIOS); anything else is a
 * real failure. The og distinguishes exactly these (its probe checks
 * GetLastError() != 1 after a zero-byte read, EFIBoot :3881-3885). */
u32 map_get_error(DWORD gle) {
    if (gle == ERROR_FILE_NOT_FOUND)
        return ERR_OPEN_FAILED; /* variable not present — the benign case */
    if (gle == ERROR_INVALID_FUNCTION)
        return ERR_UNSUPPORTED; /* no UEFI firmware at all */
    return ERR_READ_FAILED;
}

} // namespace

/* ---- FwVarStore -------------------------------------------------------- */

FwVarStore::FwVarStore() {
    /* EFIBoot resolves the pair dynamically (:3281-3290); kernel32 is
     * always loaded, so GetModuleHandleW suffices — no new import. */
    const HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
    if (k32 != nullptr) {
        get_ = reinterpret_cast<u32 (*)(const wchar_t*, const wchar_t*, void*, u32, u32*)>(
            GetProcAddress(k32, "GetFirmwareEnvironmentVariableW"));
        set_ = reinterpret_cast<bool (*)(const wchar_t*, const wchar_t*, const void*, u32)>(
            GetProcAddress(k32, "SetFirmwareEnvironmentVariableW"));
    }
    if (get_ != nullptr && set_ != nullptr)
        privileged_ = enable_privilege(SE_SYSTEM_ENVIRONMENT_NAME);
}

u32 FwVarStore::get(const wchar_t* name, void* buf, u32 buf_len, u32* out_len) {
    if (get_ == nullptr || !privileged_)
        return ERR_UNSUPPORTED;
    const DWORD n = get_(name, nvramfix::kEfiGlobalGuid, buf, buf_len, nullptr);
    if (n == 0)
        return map_get_error(GetLastError());
    if (out_len != nullptr)
        *out_len = n;
    return OK;
}

u32 FwVarStore::set(const wchar_t* name, const void* buf, u32 len) {
    if (set_ == nullptr || !privileged_)
        return ERR_UNSUPPORTED;
    if (!set_(name, nvramfix::kEfiGlobalGuid, buf, len)) {
        const DWORD gle = GetLastError();
        if (gle == ERROR_INVALID_FUNCTION)
            return ERR_UNSUPPORTED;
        return ERR_WRITE_FAILED;
    }
    return OK;
}

/* ---- the NVRAM renumber pass ------------------------------------------- */

u32 nvram_renumber(const nvramfix::Target& t, nvramfix::UpdateStats* stats) {
    if (stats != nullptr)
        *stats = nvramfix::UpdateStats{};
    FwVarStore store;
    if (!store.is_usable())
        return ERR_UNSUPPORTED; /* BIOS system: clean refusal, zero stats */
    return nvramfix::update_boot_entries(store, t, stats);
}

/* ---- the BCD file rewriter --------------------------------------------- */

u32 bcd_rewrite_file(const wchar_t* bcd_path, u32 mbr_signature, u64 old_start_lba,
                     u64 new_start_lba, bcdfix::ConsentFn consent, void* consent_user,
                     size_t* applied) {
    if (applied != nullptr)
        *applied = 0;
    if (bcd_path == nullptr)
        return ERR_BAD_PARAMETER;

    /* the og's open (Fixup :521-523): GENERIC_READ|GENERIC_WRITE, full
     * share, OPEN_EXISTING. A locked open (the loader's mounted hive)
     * is the caller's signal to run unload_bcd_hive() and retry — this
     * function stays single-purpose. */
    HANDLE h = CreateFileW(bcd_path, GENERIC_READ | GENERIC_WRITE, 3, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return ERR_OPEN_FAILED;

    std::vector<u8> hive;
    {
        LARGE_INTEGER sz = {};
        if (!GetFileSizeEx(h, &sz) || sz.QuadPart < 0 ||
            u64(sz.QuadPart) > (u64(1) << 30)) { /* sanity cap: 1 GiB */
            CloseHandle(h);
            return ERR_BAD_PARAMETER;
        }
        hive.resize(size_t(sz.QuadPart));
        DWORD got = 0;
        size_t at = 0;
        while (at < hive.size()) {
            const DWORD want = DWORD(hive.size() - at > 0x40000000 ? 0x40000000
                                                                   : hive.size() - at);
            if (!ReadFile(h, hive.data() + at, want, &got, nullptr) || got == 0) {
                CloseHandle(h);
                return ERR_READ_FAILED;
            }
            at += got;
        }
    }

    const std::vector<bcdfix::BcdRewrite> plan =
        bcdfix::plan_rewrites(hive.data(), hive.size(), mbr_signature, old_start_lba,
                              new_start_lba);
    if (plan.empty()) { /* no cell references this partition */
        CloseHandle(h);
        return OK; /* clean no-op, zero applied */
    }

    /* the writer: SetFilePointer + WriteFile per rewrite (the og's
     * FUN_180002520 seek-back/write shape, expressed through plain
     * Win32 file positioning). */
    struct FileWriteCtx {
        HANDLE h;
    } ctx{h};
    const auto writer = [](u64 field_offset, const u8 bytes[8], void* user) -> u32 {
        const HANDLE fh = static_cast<FileWriteCtx*>(user)->h;
        LARGE_INTEGER pos;
        pos.QuadPart = static_cast<long long>(field_offset);
        if (SetFilePointer(fh, pos.LowPart, &pos.HighPart, FILE_BEGIN) ==
                INVALID_SET_FILE_POINTER &&
            GetLastError() != NO_ERROR)
            return ERR_WRITE_FAILED;
        DWORD wrote = 0;
        if (!WriteFile(fh, bytes, 8, &wrote, nullptr) || wrote != 8)
            return ERR_WRITE_FAILED;
        return OK;
    };

    size_t done = 0;
    const u32 rc = bcdfix::execute_plan(plan, writer, &ctx, consent, consent_user, &done);
    if (applied != nullptr)
        *applied = done;
    /* the flush tail (Fixup :263-306): whatever execute did, the file
     * is flushed before close so a later failure cannot lose a landed
     * write. */
    FlushFileBuffers(h);
    CloseHandle(h);
    return rc;
}

/* ---- the hive force-unload --------------------------------------------- */

bool unload_bcd_hive() {
    /* Fixup :527-546: SeBackupPrivilege + SeRestorePrivilege, then
     * ZwUnloadKey over \Registry\Machine\BCD00000000. ntdll is always
     * loaded; ZwUnloadKey's signature is
     * NTSTATUS ZwUnloadKey(POBJECT_ATTRIBUTES). OBJECT_ATTRIBUTES is
     * declared locally (no winternl dependency — the SDK header drags
     * in more surface than the Win7 gate wants to bless). */
    if (!enable_privilege(SE_BACKUP_NAME) || !enable_privilege(SE_RESTORE_NAME))
        return false;

    const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll == nullptr)
        return false;
    using ZwUnloadKeyFn = long (*)(void* /* OBJECT_ATTRIBUTES* */);
    auto unload = reinterpret_cast<ZwUnloadKeyFn>(GetProcAddress(ntdll, "ZwUnloadKey"));
    if (unload == nullptr)
        return false;

    struct UStr {
        USHORT Length;
        USHORT MaximumLength;
        wchar_t* Buffer;
    } name = {};
    wchar_t path[] = L"\\Registry\\Machine\\BCD00000000";
    name.Buffer = path;
    name.Length = USHORT(sizeof(path) - sizeof(path[0])); /* excl. NUL */
    name.MaximumLength = USHORT(sizeof(path));

    struct ObjAttr {
        ULONG Length;
        HANDLE RootDirectory;
        UStr* ObjectName;
        ULONG Attributes;
        void* SecurityDescriptor;
        void* SecurityQualityOfService;
    } oa = {};
    oa.Length = sizeof(oa);
    oa.ObjectName = &name;
    oa.Attributes = 0x40 /* OBJ_CASE_INSENSITIVE */;

    return unload(&oa) == 0; /* STATUS_SUCCESS */
}

} // namespace bootfix
} // namespace win
} // namespace cr
